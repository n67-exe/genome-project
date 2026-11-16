#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace std;

struct MappedRead
{
	int count = 0;
	int position = 0;
};

class GenomeMapper {
public:
	explicit GenomeMapper(string genome)
		: bwt(genome.size() + 1, 0), inverse_map(genome.size() + 1)
	{
		genome.push_back('$');

		int n = genome.size();

		string_view genome_view(genome);

		for (auto& e : occurrences)
			e.resize(n + 1);
		
		ranges::iota(inverse_map, 0);
		
		auto cmp = [&n, &genome_view](int a, int b) -> bool
		{
			if (a < b)
			{
				int r = n - b, m = b - a;

				tuple sa(genome_view.substr(a, r), genome_view.substr(a + r, n), genome_view.substr(0, a));
				tuple sb(genome_view.substr(b, n), genome_view.substr(0, m), genome_view.substr(m, b));

				return sa < sb;
			}

			if (b < a)
			{
				int r = n - a, m = a - b;

				tuple sa(genome_view.substr(a, n), genome_view.substr(0, m), genome_view.substr(m, a));
				tuple sb(genome_view.substr(b, r), genome_view.substr(b + r, n), genome_view.substr(0, b));

				return sa < sb;
			}

			return false;
		};

		ranges::sort(inverse_map, cmp);

		for (int i = 0; i < n; ++i)
			bwt[i] = genome[(inverse_map[i] ? inverse_map[i] : n) - 1];

		for (int i = 0; i < 5; ++i)
			for (int j = 0; j < n; ++j)
				occurrences[i][j + 1] = occurrences[i][j] + (bwt[j] == orig[i]);

		for (int i = 1; i < 5; ++i)
			smaller[i] = smaller[i - 1] + occurrences[i - 1].back();
	}

	GenomeMapper(const GenomeMapper &) = default;
	GenomeMapper &operator=(const GenomeMapper &) = default;
	GenomeMapper(GenomeMapper &&) = default;
	GenomeMapper &operator=(GenomeMapper &&) = default;

	[[nodiscard]]
	MappedRead map_read(string_view read) const
	{
		// [first, second)
		auto extend = [this](pair<int, int>& range, char c)
		{
			if (c == 'N')
				return;

			range.first = smaller[id(c)] + occurrences[id(c)][range.first];
			range.second = smaller[id(c)] + occurrences[id(c)][range.second];
		};

		pair<int, int> range = { 0, (int)bwt.size() };

		for (int j = (int)read.size() - 1; j >= 0; --j)
		{
			extend(range, read[j]);

			if (range.first >= range.second)
				return MappedRead(0);
		}

		int pos = inverse_map[range.first];

		return MappedRead(range.second - range.first, pos);
	}

private:
	static constexpr int id(char c)
	{
		switch (c)
		{
			case '$': return 0;
			case 'A': return 1;
			case 'C': return 2;
			case 'G': return 3;
			case 'T': return 4;
			// case 'N': return 5;
		}

		unreachable();
	}

	static constexpr char orig[] = {'$', 'A', 'C', 'G', 'T'};

	string bwt;
	array<int, 5> smaller{};

	vector<int> inverse_map;
	array<vector<int>, 5> occurrences;
};

inline string read_genome(const filesystem::path &path)
{
	ifstream input(path);

	if (!input) {
		throw runtime_error("Could not open genome file: " + path.string());
	}

	// This file has one FASTA record: a header followed by sequence lines.
	// Multiple records and malformed input are not handled here.
	string line;
	getline(input, line); // Skip the header.

	string genome;
	while (getline(input, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		genome += line;
	}
	return genome;
}

class ReadsParser {
public:
	// Building the index scans the file once. The optional count is only a
	// capacity hint; the actual count is determined from the file.
	explicit ReadsParser(const filesystem::path &path, size_t expected_reads = 0)
		: file_(path), input_(path, ios::binary)
	{
		if (!input_)
			throw runtime_error("Could not open reads file: " + path.string());

		positions_.reserve(expected_reads);
		build_index();
	}

	ReadsParser(const ReadsParser& other)
		: file_(other.file_), input_(other.file_, ios::binary), positions_(other.positions_)
	{}

	ReadsParser &operator=(const ReadsParser& other)
	{
		if (this == &other)
			return *this;

		file_ = other.file_;
		positions_ = other.positions_;

		input_.close();
		input_.clear();
		input_.open(other.file_, ios::binary);

		return *this;
	}

	ReadsParser(ReadsParser &&) = default;
	ReadsParser &operator=(ReadsParser &&) = default;

	[[nodiscard]] size_t count_reads() const
	{
		return positions_.size();
	}

	// first_read is a zero-based read number, not a byte offset. The file
	// must not be modified after the index has been built.
	void get_reads(vector<string>& out_reads, size_t offset = 0)
	{
		if (offset >= positions_.size() || out_reads.size() == 0) {
			return out_reads.clear();
		}

		if (positions_.size() - offset < out_reads.size())
			out_reads.resize(positions_.size() - offset);

		input_.clear(); // Clear flags
		input_.seekg(positions_[offset], ios::beg);

		if (!input_)
			throw runtime_error("Could not seek to read");

		// Adjacent reads can be read sequentially
		string ignored, sequence;
		string& header = ignored, separator = ignored, quality = ignored;
		for (auto& e : out_reads) {
			if (!getline(input_, header) || !getline(input_, sequence)
				|| !getline(input_, separator) || !getline(input_, quality))
				throw runtime_error("Could not read FASTQ record");

			if (!sequence.empty() && sequence.back() == '\r')
				sequence.pop_back();

			e.swap(sequence);
		}
	}

private:
	void build_index()
	{
		// The supplied FASTQ has four unwrapped lines per record. We count
		// newlines in binary mode so offsets are correct for LF and CRLF.
		// Headers, separators and qualities are not otherwise validated.
		vector<char> buffer(64 * 1024);
		streamoff chunk_offset = 0;
		streamoff record_start = 0;
		int lines_in_record = 0;
		bool has_data = false;
		char last_byte = '\0';

		for (;;) {
			input_.read(buffer.data(), static_cast<streamsize>(buffer.size()));
			const streamsize bytes = input_.gcount();
			if (bytes > 0) {
				has_data = true;
				last_byte = buffer[static_cast<size_t>(bytes) - 1];
				const char *cursor = buffer.data();
				const char *end = cursor + bytes;
				while (cursor < end) {
					const auto *newline = static_cast<const char *>(
						memchr(cursor, '\n', static_cast<size_t>(end - cursor)));
					if (!newline) {
						break;
					}
					if (++lines_in_record == 4) {
						positions_.push_back(record_start);
						record_start = chunk_offset + (newline - buffer.data()) + 1;
						lines_in_record = 0;
					}
					cursor = newline + 1;
				}
				chunk_offset += bytes;
			}
			if (input_.bad()) {
				throw runtime_error("Could not read FASTQ file while indexing");
			}
			if (input_.eof()) {
				break;
			}
			if (!input_) {
				throw runtime_error("Could not read FASTQ file while indexing");
			}
		}

		// The last quality line may end at EOF without a newline.
		if (has_data && last_byte != '\n' && ++lines_in_record == 4) {
			positions_.push_back(record_start);
			lines_in_record = 0;
		}
		if (lines_in_record != 0) {
			throw runtime_error("Incomplete FASTQ record");
		}
	}

	filesystem::path file_;
	ifstream input_;
	vector<streamoff> positions_;
};
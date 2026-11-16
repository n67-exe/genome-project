#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <source_location>

using namespace std;

struct MappedRead
{
	int count = 0;
	int position = 0;
};

class GenomeMapper {
public:
	explicit GenomeMapper()
	{
	}

	GenomeMapper(const GenomeMapper &) = default;
	GenomeMapper &operator=(const GenomeMapper &) = default;
	GenomeMapper(GenomeMapper &&) = default;
	GenomeMapper &operator=(GenomeMapper &&) = default;

	[[nodiscard]]
	MappedRead map_read(string_view read) const
	{
		throw logic_error("Read mapping is not implemented yet");
	}

	void preprocess_genome(string_view genome)
	{

	}

private:

	string genome_;
};

string read_genome(const filesystem::path &path)
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
	void get_reads(size_t first_read, vector<string>& out_reads)
	{
		if (first_read >= positions_.size() || out_reads.size() == 0) {
			return out_reads.clear();
		}

		if (positions_.size() - first_read < out_reads.size())
			out_reads.resize(positions_.size() - first_read);

		input_.clear(); // Clear flags
		input_.seekg(positions_[first_read], ios::beg);

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

int main(int argc, char *argv[])
{
	const filesystem::path current = source_location::current().file_name();

	const filesystem::path genome_path = current.parent_path() / "data" / "GCF_000005845.2_ASM584v2_genomic.fna";
	const filesystem::path reads_path = current.parent_path() / "data" / "ERR022075_1.fastq";

	try {
		ReadsParser parser(reads_path, 22'720'100);
		cout << parser.count_reads() << endl;
		// string genome = read_genome(genome_path);
		// const size_t genome_size = genome.size();
		// GenomeMapper mapper;
		// mapper.preprocess_genome(genome);
		// const auto reads = parser.read_reads(0, 10);
		//
		// cout << "Loaded " << genome_size << " genome bases and "
		//           << reads.size() << " reads.\n";
		// Call mapper.map_read(reads[i]) once mapping is implemented.
	} catch (const exception &error) {
		cerr << "Caught std::exception:\n" << error.what() << '\n';
		return 1;
	}
	return 0;
}

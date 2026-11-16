#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
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

size_t count_reads(const filesystem::path &path)
{
	ifstream input(path);
	if (!input) {
		throw runtime_error("Could not open reads file: " + path.string());
	}

	// Assumes the supplied FASTQ format: four unwrapped lines per read.
	// Counting requires a complete pass through the file.
	size_t line_count = 0;
	string line;
	while (getline(input, line)) {
		++line_count;
	}
	if (input.bad()) {
		throw runtime_error("Could not read reads file: " + path.string());
	}
	return line_count / 4;
}

vector<string> read_reads(const filesystem::path &path,
                                    size_t max_reads)
{
	ifstream input(path);
	if (!input) {
		throw runtime_error("Could not open reads file: " + path.string());
	}

	vector<string> reads;
	string header, sequence, separator, quality;

	// This FASTQ is assumed to have one line each for the header, sequence,
	// separator and quality. Identifiers, qualities and wrapped records are
	// not needed by this example and are not processed or validated.
	while (reads.size() < max_reads && getline(input, header)) {
		if (!getline(input, sequence) || !getline(input, separator) ||
			!getline(input, quality)) {
			break;
		}
		if (!sequence.empty() && sequence.back() == '\r') {
			sequence.pop_back();
		}
		reads.push_back(move(sequence));
	}
	return reads;
}

int main(int argc, char *argv[])
{
	const filesystem::path current = source_location::current().file_name();

	const filesystem::path genome_path = current.parent_path() / "data" / "GCF_000005845.2_ASM584v2_genomic.fna";
	const filesystem::path reads_path = current.parent_path() / "data" / "ERR022075_1.fastq";

	try {
		cout << count_reads(reads_path) << endl;
		// string genome = read_genome(genome_path);
		// const size_t genome_size = genome.size();
		// GenomeMapper mapper;
		// mapper.preprocess_genome(genome);
		// const auto reads = read_reads(reads_path, 10);
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

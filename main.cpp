#include <source_location>

#include "main.hpp"

int main(int argc, char *argv[])
{
	const filesystem::path current = source_location::current().file_name();

	const filesystem::path genome_path = current.parent_path() / "data" / "GCF_000005845.2_ASM584v2_genomic.fna";
	const filesystem::path reads_path = current.parent_path() / "data" / "ERR022075_1.fastq";

	try {
		ReadsParser parser(reads_path, 22'720'100);
		cout << parser.count_reads() << endl;

		vector<string> reads(15);

		parser.get_reads(reads);

		for (auto& read : reads)
		{
			cout << read << endl;
		}
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

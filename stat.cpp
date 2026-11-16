#include <source_location>

#include "main.hpp"

int main(int argc, char *argv[])
{
	const filesystem::path current = source_location::current().file_name();

	const filesystem::path genome_path = current.parent_path() / "data" / "GCF_000005845.2_ASM584v2_genomic.fna";
	const filesystem::path reads_path = current.parent_path() / "data" / "ERR022075_1.fastq";

	try
	{
		vector<string> reads(1024);

		ReadsParser parser(reads_path, 22'720'100);

		cout << "Indexing done" << endl;

		cout << string((parser.count_reads() - 1) / (1024 * 500) + 1, '_') << endl;

		size_t count = 0, cmin = 999, cmax = 0;
		size_t with = 0, without = 0;

		for (size_t offset = 0; reads.size(); offset += reads.size())
		{
			if (offset % (1024 * 500) == 0)
				cout << '#' << flush;

			parser.get_reads(reads, offset);

			for (auto &read : reads)
			{
				size_t c = ranges::count(read, 'N');

				cmin = min(cmin, c);
				cmax = max(cmax, c);
				count += c;

				c ? ++with : ++without;
			}
		}

		cout << endl;

		cout << parser.count_reads() << endl;

		cout << "Count: " << count << endl;
		cout << "Min:   " << cmin << endl;
		cout << "Max:   " << cmax << endl;
		cout << "W/:    " << with << endl;
		cout << "W/o:   " << without << endl;
	}
	catch (const exception &error)
	{
		cerr << "Caught std::exception:\n" << error.what() << '\n';
		return 1;
	}
	return 0;
}

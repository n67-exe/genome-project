#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <sstream>
#include <string>
#include <vector>

#include "main.hpp"

using namespace std;

using boolean = unsigned char;

int main(int argc, char *argv[])
{
	const filesystem::path current = source_location::current().file_name();

	const filesystem::path genome_path = current.parent_path() / "data" / "GCF_000005845.2_ASM584v2_genomic.fna";
	const filesystem::path reads_path = current.parent_path() / "data" / "ERR022075_1.fastq";
	const filesystem::path input_path = current.parent_path() / "data" / "exact_all_2.txt";

	ifstream input(input_path);

	if (!input)
	{
		cerr << "Could not open mapping output: " << input_path << '\n';
		return 1;
	}

	ReadsParser parser(reads_path);

	cout << "Indexing done" << endl;

	size_t line_number = 0;

	string line, label;

	size_t skipd = 0, unmapp = 0, mapp = 0, mult = 0;
	vector<boolean> coverage(read_genome(genome_path).length(), false);

	vector<string> reads(1);

	while (getline(input, line))
	{
		++line_number;
		istringstream row(line);

		long long row_index = line_number - 1;
		int ambiguity;

		if (!(row >> row_index) || row_index < 0 || row_index != line_number - 1)
		{
			cerr << "Invalid row index at line " << line_number << '\n';
			return 1;
		}

		if (!(row >> ambiguity))
		{
			cerr << "Invalid ambiguity at line " << line_number << '\n';
			return 1;
		}

		if (!(row >> label))
		{
			cerr << "Missing label at row " << row_index << '\n';
			return 1;
		}

		if (label == "skipped")
		{
			skipd++;
		}
		else if (label == "missing")
		{
			unmapp++;
		}
		else if (label == "single")
		{
			int index;
			if (!(row >> index) || index < 0)
			{
				cerr << "Invalid single record at row " << row_index << '\n';
				return 1;
			}

			mapp++;

			parser.get_reads(reads, index);

			for (int j = 0; j < reads.front().size(); ++j)
				coverage[index + j] = true;

			// TODO: Analyze the read at row_index mapped to index.
		}
		else if (label == "multiple")
		{
			int count;
			if (!(row >> count) || count < 2)
			{
				cerr << "Invalid multiple count at row " << row_index << '\n';
				return 1;
			}

			vector<int> indices(count);

			for (int &index : indices)
			{
				if (!(row >> index) || index < 0)
				{
					cerr << "Missing or invalid mapping index at row " << row_index << '\n';
					return 1;
				}

				// TODO: Analyze this mapping index for the read at row_index.
			}

			mult++;
		}
		else
		{
			cerr << "Unknown label at row " << row_index << ": " << label << '\n';
			return 1;
		}

		string extra;
		if (row >> extra)
		{
			cerr << "Unexpected extra field at row " << row_index << ": " << extra << '\n';
			return 1;
		}
	}

	if (input.bad())
	{
		cerr << "Could not read mapping output: " << input_path << '\n';
		return 1;
	}

	cout << skipd << ' ' << unmapp << ' ' << mapp << ' ' << mult << endl;

	cout << "Coverage: " << ranges::count(coverage, true) << endl;

	return 0;
}

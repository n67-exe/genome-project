#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <string>

using namespace std;

int main(int argc, char *argv[])
{
	if (argc > 2)
	{
		cerr << "Usage: analysis [mapping-output]\n";
		return 1;
	}

	const filesystem::path source = source_location::current().file_name();
	const filesystem::path input_path = argc == 2
		? filesystem::path(argv[1])
		: source.parent_path() / "data" / "wildcards_all_1.txt";

	ifstream input(input_path);
	if (!input)
	{
		cerr << "Could not open mapping output: " << input_path << '\n';
		return 1;
	}

	string label;
	size_t record = 0;

	int maxm = 0;

	while (input >> label)
	{
		++record;

		if (label == "skipped")
		{
			// TODO: Analyze skipped reads.
		}
		else if (label == "missing")
		{
			// TODO: Analyze missing reads.
		}
		else if (label == "single")
		{
			int position;
			if (!(input >> position))
			{
				cerr << "Invalid single record at record " << record << '\n';
				return 1;
			}

			// TODO: Analyze reads mapped once, using position.
		}
		else if (label == "multiple")
		{
			int count, position;
			if (!(input >> count >> position))
			{
				cerr << "Invalid multiple record at record " << record << '\n';
				return 1;
			}

			// TODO: Analyze reads mapped multiple times, using count and position.

			maxm = max(maxm, count);
		}
		else
		{
			cerr << "Unknown label at record " << record << ": " << label << '\n';
			return 1;
		}

		input.ignore(32768, '\n');
	}

	if (input.bad() || !input.eof())
	{
		cerr << "Could not read mapping output: " << input_path << '\n';
		return 1;
	}

	cout << "Max multiple: " << maxm << endl;
}

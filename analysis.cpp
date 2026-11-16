#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <string>

int main(int argc, char *argv[])
{
	if (argc > 2)
	{
		std::cerr << "Usage: analysis [mapping-output]\n";
		return 1;
	}

	const std::filesystem::path source = std::source_location::current().file_name();
	const std::filesystem::path input_path = argc == 2
		? std::filesystem::path(argv[1])
		: source.parent_path() / "data" / "exact_1.txt";

	std::ifstream input(input_path);
	if (!input)
	{
		std::cerr << "Could not open mapping output: " << input_path << '\n';
		return 1;
	}

	std::string label;
	std::size_t record = 0;
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
				std::cerr << "Invalid single record at record " << record << '\n';
				return 1;
			}

			// TODO: Analyze reads mapped once, using position.
		}
		else if (label == "multiple")
		{
			int count, position;
			if (!(input >> count >> position))
			{
				std::cerr << "Invalid multiple record at record " << record << '\n';
				return 1;
			}

			// TODO: Analyze reads mapped multiple times, using count and position.
		}
		else
		{
			std::cerr << "Unknown label at record " << record << ": " << label << '\n';
			return 1;
		}
	}

	if (input.bad() || !input.eof())
	{
		std::cerr << "Could not read mapping output: " << input_path << '\n';
		return 1;
	}

	return 0;
}

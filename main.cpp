#include <atomic>
#include <chrono>
#include <exception>
#include <omp.h>
#include <source_location>

#include "main.hpp"

int main(int argc, char *argv[])
{
	const filesystem::path current = source_location::current().file_name();

	const filesystem::path genome_path = current.parent_path() / "data" / "GCF_000005845.2_ASM584v2_genomic.fna";
	const filesystem::path reads_path = current.parent_path() / "data" / "ERR022075_1.fastq";
	const filesystem::path output_path = current.parent_path() / "data" / "wildcards_all_2";

	using steady_clock = chrono::steady_clock;

	steady_clock::time_point total_start = steady_clock::now();;

	try
	{
		const auto preprocessing_start = steady_clock::now();
		GenomeMapper mapper(read_genome(genome_path));
		const auto preprocessing_time = steady_clock::now() - preprocessing_start;

		cout << "Preprocessing done in " << chrono::duration<double>(preprocessing_time).count() << " seconds" << endl;

		const auto indexing_start = steady_clock::now();
		ReadsParser parser(reads_path, 22'720'100);
		const auto indexing_time = steady_clock::now() - indexing_start;

		cout << "Indexing done in " << chrono::duration<double>(indexing_time).count() << " seconds" << endl;

		constexpr int thread_count = 8;
		constexpr size_t batch_size = 1024;
		constexpr size_t report_interval = batch_size * 500;

		const size_t read_count = parser.count_reads();

		cout << string(read_count == 0 ? 0 : (read_count - 1) / report_interval + 1, '_') << endl;

		filesystem::create_directories(output_path);

		atomic<bool> failed = false;
		exception_ptr failure;
		omp_set_dynamic(0);

		#pragma omp parallel num_threads(thread_count) firstprivate(mapper, parser, read_count) shared(cout, output_path, failed, failure) default(none)
		{
			vector<string> reads;
			vector<int> indices;

			ofstream out;

			try
			{
				reads.resize(batch_size);
				indices.reserve(32);

				const filesystem::path path = output_path / (to_string(omp_get_thread_num()) + ".txt");

				out.open(path);

				if (!out)
					throw runtime_error("Could not open output file: " + path.string());

				out.exceptions(ios::badbit | ios::failbit);
			}
			catch (...)
			{
				#pragma omp critical(init_error)
				{
					if (!failure)
						failure = current_exception();
				}
				failed.store(true);
			}

			#pragma omp barrier

			#pragma omp for schedule(dynamic, 1)
			for (size_t offset = 0; offset < read_count; offset += batch_size)
			{
				if (failed.load())
					continue;

				try
				{
					parser.get_reads(reads, offset);

					if (offset % report_interval == 0)
					{
						#pragma omp critical(progress)
						cout << '#' << flush;
					}

					for (size_t i = 0; i < reads.size(); ++i)
					{
						mapper.map_read(reads[i], indices);

						// Use read numbers to reconstruct the original output order
						out << offset + i << ' ';

						if (indices.empty())
							out << "missing\n";
						else if (indices.size() == 1)
							out << "single " << indices.front() << '\n';
						else
						{
							out << "multiple " << indices.size();

							for (int ind : indices)
								out << ' ' << ind;

							out << '\n';
						}
					}
				}
				catch (...)
				{
					#pragma omp critical(mapping_error)
					{
						if (!failure)
							failure = current_exception();
					}
					failed.store(true);
				}
			}
		}

		if (failure)
			rethrow_exception(failure);

		cout << endl;
	}
	catch (const exception &error)
	{
		cerr << "Caught std::exception:\n" << error.what() << '\n';

		const auto total_time = steady_clock::now() - total_start;
		cout << "Program finished in " << chrono::duration<double>(total_time).count() << " seconds" << endl;

		return 1;
	}

	const auto total_time = steady_clock::now() - total_start;
	cout << "Program finished in " << chrono::duration<double>(total_time).count() << " seconds" << endl;

	return 0;
}

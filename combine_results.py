#!/usr/bin/env python3
"""Combine the numbered worker files produced by main.cpp.

Usage: python combine_results.py data/wildcards_all_2 data/combined.txt

Rows keep their original indices and are sorted numerically. Sorted worker
files are merged directly; otherwise, bounded-memory external sorting is used.
An existing output file is replaced after the merge succeeds.
"""

import argparse
import heapq
import os
from operator import itemgetter
from pathlib import Path
import re
import sys
import tempfile
from contextlib import ExitStack


WORKER_FILE = re.compile(r"[0-9]+\.txt\Z")
RUN_SIZE = 64 * 1024 * 1024
MERGE_FAN_IN = 64


class UnsortedInput(Exception):
    pass


def records(file, path, check_order=False):
    previous = -1
    for line_number, line in enumerate(file, 1):
        parts = line.split(None, 1)
        if len(parts) != 2 or not parts[0].isdigit():
            raise ValueError(f"Invalid row index or missing result in {path}, line {line_number}")
        try:
            index = int(parts[0])
        except ValueError as error:
            raise ValueError(f"Invalid row index in {path}, line {line_number}") from error
        if check_order and index < previous:
            raise UnsortedInput
        previous = index
        # A final unterminated line must not run into the next merged record.
        if not line.endswith(b"\n"):
            line += b"\n"
        yield index, line


def merge_files(paths, destination):
    with ExitStack() as stack:
        streams = [
            records(stack.enter_context(path.open("rb")), path, check_order=True)
            for path in paths
        ]
        output = stack.enter_context(destination.open("wb"))
        for _, line in heapq.merge(*streams, key=itemgetter(0)):
            output.write(line)


def merge_in_batches(paths, destination, temporary_dir, prefix, owned=False):
    """Limit the number of open files, deleting intermediate runs as we go."""
    paths = list(paths)
    pass_number = 0
    while len(paths) > MERGE_FAN_IN:
        next_paths = []
        for start in range(0, len(paths), MERGE_FAN_IN):
            batch = paths[start:start + MERGE_FAN_IN]
            merged = temporary_dir / f"{prefix}-{pass_number}-{start}.tmp"
            merge_files(batch, merged)
            next_paths.append(merged)
            if owned:
                for path in batch:
                    path.unlink()
        paths = next_paths
        owned = True
        pass_number += 1
    merge_files(paths, destination)
    if owned:
        for path in paths:
            path.unlink()


def sort_in_chunks(paths, destination, temporary_dir):
    runs = []
    chunk = []
    chunk_size = 0

    def flush_chunk():
        nonlocal chunk_size
        if not chunk:
            return
        chunk.sort(key=itemgetter(0))
        path = temporary_dir / f"run-{len(runs)}.tmp"
        with path.open("wb") as output:
            for _, line in chunk:
                output.write(line)
        runs.append(path)
        chunk.clear()
        chunk_size = 0

    for path in paths:
        with path.open("rb") as file:
            for index, line in records(file, path):
                chunk.append((index, line))
                # Account for the row, integer, tuple, list entry and sorting overhead.
                chunk_size += len(line) + 128
                if chunk_size >= RUN_SIZE:
                    flush_chunk()
    flush_chunk()
    merge_in_batches(runs, destination, temporary_dir, "sorted", owned=True)


def combine(input_dir, output_path):
    if not input_dir.is_dir():
        raise ValueError(f"Input directory does not exist: {input_dir}")
    if not output_path.parent.is_dir():
        raise ValueError(f"Output directory does not exist: {output_path.parent}")
    if output_path.is_dir():
        raise ValueError(f"Output is a directory: {output_path}")

    paths = sorted(
        path for path in input_dir.iterdir()
        if WORKER_FILE.fullmatch(path.name) and path.is_file()
    )
    if not paths:
        raise ValueError(f"No numbered worker files found in {input_dir}")
    if (output_path.parent.resolve() == input_dir.resolve()
            and WORKER_FILE.fullmatch(output_path.name)):
        raise ValueError("Output must not have a numbered worker filename inside the input directory")
    if output_path.exists() and any(os.path.samefile(output_path, path) for path in paths):
        raise ValueError("Output must not overwrite an input file")

    # Keep temporary files next to the destination for an atomic replacement.
    with tempfile.TemporaryDirectory(prefix=".combine-", dir=output_path.parent) as directory:
        temporary_dir = Path(directory)
        merged = temporary_dir / "result.tmp"
        try:
            merge_in_batches(paths, merged, temporary_dir, "direct")
        except UnsortedInput:
            print("Worker files are out of order; sorting in bounded-memory chunks.", file=sys.stderr)
            # Remove partial direct-merge runs before starting the external sort.
            merged.unlink(missing_ok=True)
            for path in temporary_dir.glob("direct-*.tmp"):
                path.unlink()
            sort_in_chunks(paths, merged, temporary_dir)
        os.replace(merged, output_path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input_dir", type=Path, help="directory containing numbered worker .txt files")
    parser.add_argument("output", type=Path, help="path for the combined output")
    args = parser.parse_args()
    try:
        combine(args.input_dir, args.output)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Error: {error}\n")


if __name__ == "__main__":
    main()

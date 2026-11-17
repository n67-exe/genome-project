# Genome Mapping

## Algorithms used
- Burrows–Wheeler transform (BWT)
- FM-index

## Exact matches
Stats for exact matches skipping reads that contain `N`.
- `22720100` reads processed
- `265092/22720100` (1.2%) skipped
- `12604448/22720100` (55.5%) not mapped
- `9583017/22720100` (42.2%) mapped to a single location
- `267543/22720100` (1.2%) mapped to multiple locations
- `4570472/4641652` (98.5%) coverage
- `30s - 40s` analysis time (10 threads)

## Exact matches with wildcards
Stats for exact matches taking `N` as wildcards. 
- `22720100` reads processed 
- `12751441/22720100` (56.1%) not mapped
- `9697541/22720100` (42.7%) mapped to a single location 
- `271118/22720100` (1.2%) mapped to multiple locations
- `4570496/4641652` (98.5%) coverage
- `300s - 400s` analysis time (10 threads)

## Approximate (distance 1) matches with wildcards [run finished after the deadline]
Stats for approximate matches that are 1 Hamming distance away taking `N` as wildcards.
- `22720100` reads processed
- `11804772/22720100` (52.0%) not mapped
- `10582911/22720100` (46.6%) mapped to a single location
- `332417/22720100` (1.5%) mapped to multiple locations
- `4570806/4641652` (98.5%) coverage
- `~10000s` analysis time (10 threads)

Output files with all the calculated mappings are not included in the repository due to their size (~500MB each).
There are available to download [here](https://drive.google.com/file/d/10gbDECNni9Ntq2Iw3r4ossx_vCtKV4tt/view).

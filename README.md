# Genome Mapping

## Algorithms used
- Burrows–Wheeler transform (BWT)
- FM-index

## Stats (exact)
For exact matches skipping reads that contain `N`.
- `22720100` reads processed
- `265092/22720100` (1.2%) skipped
- `12604448/22720100` (55.5%) not mapped
- `9583017/22720100` (42.2%) mapped to a single location
- `267543/22720100` (1.2%) mapped to multiple locations
- `4570472/4641652` (98.5%) coverage

## Stats (wildcards)
For exact matches taking `N` as wildcards. 
- `22720100` reads processed 
- `12751441/22720100` (56.1%) not mapped
- `9697541/22720100` (42.7%) mapped to a single location 
- `271118/22720100` (1.2%) mapped to multiple locations
- `4570496/4641652` (98.5%) coverage
- `300s - 400s` analysis time (10 threads)

Output files with all the calculated mappings are not included here due to size constraints (~500MB each)

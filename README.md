# Genome Mapping

## Algorithms used
- Burrows–Wheeler transform (BWT)
- FM-index

## Stats (exact)
For exact matches taking `N` as wildcards. 
- `22720100` reads processed 
- `12751441/22720100` (56.1%) not mapped
- `9697541/22720100` (42.7%) mapped to a single location 
- `271118/22720100` (1.2%) mapped to multiple locations
- `4570496/4641652` (98.4%) coverage
- `300s - 400s` analysis time (10 cores)

Output file with all the calculated mappings is not included here due to size constraints (~500MB)

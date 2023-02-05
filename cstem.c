#include <immintrin.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// The input and output buffer size. The value of 84 is derived from the largest kind
// of bouquet supported by this program:
//     2 (name + size) + 26 (species) * 3 (~10 stems + name) + 3 (total >99) + 1 (\n)
#define BUFFER_SIZE 84

// The maximum number of designs supported (per size) by this program. This is based on
// the number of _unique_ names (A-Z). It is unclear whether the challenge requires
// support for non-unique names.
#define MAX_DESIGNS 26
#define NUMBER_OF_SPECIES 26

#define min(a, b) ((a) < (b) ? (a) : (b))

typedef enum Size {
    Small = 'S',
    Large = 'L',
} Size;

typedef struct Stem {
    char species;
    Size size;
} Stem;

typedef struct Design {
    __m256i min_stems;
    __m256i max_stems;
    char name;
    Size size;
    int8_t total;
} Design;

typedef struct Division {
    // Vector of 32 int8_t values. For simple accounting.
    __m256i minimal_stock;
    // This contains the actual stock numbers. On design creation the stock numbers are
    // redistributed into the simpler saturated_stock vector, where stock values can at
    // most be 255.
    // Because we don't want to convert the int16_t vectors into a int8_t vector every
    // iteration, stock is kept in both places. Only when the saturated_stock reaches
    // critical lows (e.g. when the amount of stems of a species is below the maximum
    // possible needed for a bouquet) is the stock redistributed.
    int16_t stock[NUMBER_OF_SPECIES];
    int8_t max_per_species[NUMBER_OF_SPECIES];
    Design *designs[NUMBER_OF_SPECIES];
    int8_t _designs_insert_index;
} Division;

typedef struct Warehouse {
    Division *small;
    Division *large;
} Warehouse;

// Writes a formatted error message to stderr and exits.
__attribute__((noreturn)) void panic(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Error: ");
    fprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

char parse_design_name(char input) {
    if ('A' <= input && input <= 'Z') return input;
    else panic("Invalid design name");
}
char parse_species(char input) {
    if ('a' <= input && input <= 'z') return input;
    else panic("Invalid species");
}
Size parse_size(char input) {
    if (input == 'S') return Small;
    else if (input == 'L') return Large;
    else panic("Invalid size");
}
Stem parse_stem(char *input) {
    Stem stem = {};
    stem.species = parse_species(*input++);
    stem.size = parse_size(*input++);
    if (*input == '\n' || *input == '\0') return stem;
    else panic("Stem input contains additional characters");
}

Design *new_design(char *input) {
    Design *design;
    int result = posix_memalign((void **)&design, 32, sizeof(Design));
    if (result != 0 || design == NULL) panic("Could not allocate memory for Design in new_design");
    // Parse name and size from input
    design->name = parse_design_name(*input++);
    design->size = parse_size(*input++);
    // Instantiate the SIMD vectors (32 x char each)
    design->min_stems = _mm256_setzero_si256();
    design->max_stems = _mm256_setzero_si256();
    int8_t *min_stems = (int8_t *)&design->min_stems;
    int8_t *max_stems = (int8_t *)&design->max_stems;
    // Parse species from input
    int8_t species_count = 0;
    design->total = 0;
    while (true) {
        int8_t max = strtoul(input, &input, 10);
        char species = *input++;
        if (species == '\0' || species == '\n') {
            design->total = max;
            break;
        }
        parse_species(species); // for validation purposes
        const int8_t species_index = species - 'a';
        min_stems[species_index] = 1;
        max_stems[species_index] = max;
        species_count++;
    }
    // Optimization: Set max_stems to the minimum value possible.
    // 
    //   Converts "AS6a4" to "AS4a4" and "BS9a2b9" to "BS8a1b9".
    //
    int8_t sum_of_max_species = 0;
    const int8_t max_per_species = 1 + design->total - species_count;
    for (int8_t species_index = 0; species_index < NUMBER_OF_SPECIES; species_index++) {
        const int8_t species_max = max_stems[species_index];
        const int8_t new_max = min(species_max, max_per_species);
        max_stems[species_index] = new_max;
        sum_of_max_species += new_max;
    }
    // Optimization: Set min_stems to the maximum value possible
    //
    //   Sets the minimum of a in "AS6a4" to 4. Because this design always has 4 stems.
    //   Sets the minimum of a in "BS9a2b9" to 7. Bit more complex, but we can take the
    //   sum of all other species---in this case just 'b'---and subtract that from the
    //   current maximum: 9 (max of a) - 2 (max of b) = 7.
    //
    for (int species_index = 0; species_index < NUMBER_OF_SPECIES; species_index++) {
        const int8_t species_max = max_stems[species_index];
        if (species_max != 0) {
            int8_t species_min = 1;
            const int8_t sum_of_other_species_max = sum_of_max_species - species_max;
            if (sum_of_other_species_max < design->total) {
                species_min = design->total - sum_of_other_species_max;
            }
            min_stems[species_index] = species_min;
        }
    }
    return design;
}

Division *new_division() {
    Division *division;
    int result = posix_memalign((void **)&division, 32, sizeof(Division));
    if (result != 0 || division == NULL) panic("Could not allocate memory for Division in new_division");
    memset(division->designs, 0, sizeof(Design*) * NUMBER_OF_SPECIES);
    memset(division->stock, 0, NUMBER_OF_SPECIES);
    memset(division->max_per_species, 0, NUMBER_OF_SPECIES);
    division->_designs_insert_index = 0;
    return division;
}

Warehouse *new_warehouse() {
    Warehouse *warehouse = malloc(sizeof *warehouse);
    warehouse->small = new_division();
    warehouse->large = new_division();
    return warehouse;
}

void free_division(Division *division) {
    for (int8_t index = 0; index < MAX_DESIGNS; index++) {
        Design *design = division->designs[index];
        if (design == NULL) break;
        free(design);
    }
    free(division);
}

void free_warehouse(Warehouse *warehouse) {
    free_division(warehouse->small);
    free_division(warehouse->large);
    free(warehouse);
}

Division* warehouse_division(Warehouse *warehouse, Size size) {
    if (size == 'S') return warehouse->small;
    else if (size == 'L') return warehouse->large;
    else return NULL;
}

void division_add_design(Division *division, Design *design) {
    division->designs[division->_designs_insert_index++] = design;
}

void make_division_production_ready(Division *division) {
    int8_t *max_per_species = (int8_t *)&division->max_per_species;
    for (int8_t design_index = 0; design_index < MAX_DESIGNS; design_index++) {
        Design *design = division->designs[design_index];
        if (design == NULL) break;
        for (int8_t species_index = 0; species_index < NUMBER_OF_SPECIES; species_index++) {
            int8_t *max_stems = (int8_t *)&design->max_stems;
            int8_t current_species_max = max_stems[species_index];
            int8_t all_species_max = max_per_species[species_index];
            if (current_species_max > all_species_max) {
                max_per_species[species_index] = current_species_max;
            }
        }
    }
}

void make_warehouse_production_ready(Warehouse *warehouse) {
    make_division_production_ready(warehouse->small);
    make_division_production_ready(warehouse->large);
}

void division_add_stem(Division *division, char species) {
    int8_t species_index = species - 'a';
    // Input stem into the stock
    division->stock[species_index] += 1;
    if (division->stock[species_index] > division->max_per_species[species_index]) {
        return;
    }
    ((int8_t *)&division->minimal_stock)[species_index] += 1;
    // TODO: Add species to design mapping
    for (int8_t design_index = 0; design_index < NUMBER_OF_SPECIES; design_index++) {
        Design *design = division->designs[design_index];
        if (design == NULL) break;
        __m256i hand = _mm256_min_epi8(division->minimal_stock, design->max_stems);
        int8_t *h = (int8_t *)&hand;
        int8_t amount_in_hand = (
            // Automatically optimized to SIMD operations
            h[0] + h[1] + h[2] + h[3] + h[4] + h[5] + h[6] + h[7] +
            h[8] + h[9] + h[10] + h[11] + h[12] + h[13] + h[14] + h[15] +
            h[16] + h[17] + h[18] + h[19] + h[20] + h[21] + h[22] + h[23] +
            h[24] + h[25]
        );
        if (amount_in_hand < design->total) continue;
        {
            // Check if the stock is greater than or equal to the minimum required.
            __m256i eq = _mm256_cmpeq_epi8(hand, design->min_stems);
            __m256i gt = _mm256_cmpgt_epi8(hand, design->min_stems);
            __m256i ge = _mm256_or_si256(eq, gt);
            __m256i ones = _mm256_set1_epi8(-1);
            int is_ge = _mm256_testc_si256(ge, ones);
            if (!is_ge) continue;
        }
        int8_t excess_amount = amount_in_hand - design->total;
        if (excess_amount != 0) {
            __m256i excess_stems = _mm256_sub_epi8(hand, design->min_stems);
            for (int8_t species_index = 0; species_index < 26; species_index++) {
                int8_t stem_amount = ((int8_t *)&excess_stems)[species_index];
                if (stem_amount == 0) continue;
                int8_t return_amount = min(excess_amount, stem_amount);
                h[species_index] -= return_amount;
                excess_amount -= return_amount;
                if (excess_amount == 0) break;
            }
        }
        printf("%c%c", design->name, design->size);
        for (int8_t species_index = 0; species_index < 26; species_index++) {
            int8_t amount = h[species_index];
            if (amount == 0) continue;
            division->stock[species_index] -= amount;
            ((int8_t *)&division->minimal_stock)[species_index] = min(
                division->stock[species_index],
                INT8_MAX
            );
            printf("%hhu%c", amount, (char)species_index + 'a');
        }
        puts("");
        return;
    }
}

#if false
void print_m256i(__m256i *vector) {
    int8_t *indexable_vector = (int8_t *)vector;
    printf("[%hhu", indexable_vector[0]);
    for (int i = 1; i < 32; ++i) {
        printf(", %hhu", indexable_vector[i]);
    }
    printf("]");
}
void print_design(Design *design) {
    printf(
        "Design {\n    name: '%c',\n    size: '%c',\n    total: %i",
        design->name,
        design->size,
        design->total
    );
    printf(",\n    min_stems: ");
    print_m256i(&design->min_stems);
    printf(",\n    max_stems: ");
    print_m256i(&design->max_stems);
    printf(",\n}\n");
}
#endif

// Reads a line. Returns NULL on end-of-file or empty line.
inline char *readline(char *buffer, int buffer_size, FILE *stream) {
    char *status = fgets(buffer, buffer_size, stream);
    if (status == NULL || buffer[0] == '\n') return NULL;
    return status;
}

int main() {
    char input[BUFFER_SIZE];
    Warehouse *warehouse = new_warehouse();

    // Read designs from stdin
    while (readline(input, BUFFER_SIZE, stdin) != NULL) {
        Design *design = new_design(input);
        Division *division = warehouse_division(warehouse, design->size);
        division_add_design(division, design);
    }
    make_warehouse_production_ready(warehouse);
    // Read stems from stdin and produce bouquets
    while (readline(input, BUFFER_SIZE, stdin) != NULL) {
        Stem stem = parse_stem(input);
        Division *division = warehouse_division(warehouse, stem.size);
        division_add_stem(division, stem.species);
    }

    free_warehouse(warehouse);
    return 0;
}

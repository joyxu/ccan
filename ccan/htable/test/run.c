#include <ccan/htable/htable.h>
#include <ccan/htable/htable.c>
#include <ccan/tap/tap.h>
#include <stdbool.h>
#include <string.h>

#define NUM_VALS (1 << HTABLE_BASE_BITS)

/* We use the number divided by two as the hash (for lots of
   collisions), plus set all the higher bits so we can detect if they
   don't get masked out. */
static size_t hash(const void *elem, void *unused)
{
	size_t h = *(uint64_t *)elem / 2;
	h |= -1UL << HTABLE_BASE_BITS;
	return h;
}

static bool objcmp(const void *htelem, void *cmpdata)
{
	return *(uint64_t *)htelem == *(uint64_t *)cmpdata;
}

static void add_vals(struct htable *ht,
		     const uint64_t val[], unsigned int num)
{
	uint64_t i;

	for (i = 0; i < num; i++) {
		if (htable_get(ht, hash(&i, NULL), objcmp, &i)) {
			fail("%llu already in hash", (long long)i);
			return;
		}
		htable_add(ht, hash(&val[i], NULL), &val[i]);
		if (htable_get(ht, hash(&i, NULL), objcmp, &i) != &val[i]) {
			fail("%llu not added to hash", (long long)i);
			return;
		}
	}
	pass("Added %llu numbers to hash", (long long)i);
}

#if 0
static void refill_vals(struct htable *ht,
			const uint64_t val[], unsigned int num)
{
	uint64_t i;

	for (i = 0; i < num; i++) {
		if (htable_get(ht, hash(&i, NULL), objcmp, &i))
			continue;
		htable_add(ht, hash(&val[i], NULL), &val[i]);
	}
}
#endif

static void find_vals(struct htable *ht,
		      const uint64_t val[], unsigned int num)
{
	uint64_t i;

	for (i = 0; i < num; i++) {
		if (htable_get(ht, hash(&i, NULL), objcmp, &i) != &val[i]) {
			fail("%llu not found in hash", (long long)i);
			return;
		}
	}
	pass("Found %llu numbers in hash", (long long)i);
}

static void del_vals(struct htable *ht,
		     const uint64_t val[], unsigned int num)
{
	uint64_t i;

	for (i = 0; i < num; i++) {
		if (!htable_del(ht, hash(&val[i], NULL), &val[i])) {
			fail("%llu not deleted from hash", (long long)i);
			return;
		}
	}
	pass("Deleted %llu numbers in hash", (long long)i);
}

static bool check_mask(struct htable *ht, uint64_t val[], unsigned num)
{
	uint64_t i;

	for (i = 0; i < num; i++) {
		if (((uintptr_t)&val[i] & ht->common_mask) != ht->common_bits)
			return false;
	}
	return true;
}

int main(int argc, char *argv[])
{
	unsigned int i;
	struct htable *ht;
	uint64_t val[NUM_VALS];
	uint64_t dne;
	void *p;
	struct htable_iter iter;

	plan_tests(19);
	for (i = 0; i < NUM_VALS; i++)
		val[i] = i;
	dne = i;

	ht = htable_new(hash, NULL);
	ok1(ht->max < (1 << ht->bits));
	ok1(ht->bits == HTABLE_BASE_BITS);

	/* We cannot find an entry which doesn't exist. */
	ok1(!htable_get(ht, hash(&dne, NULL), objcmp, &dne));

	/* Fill it, it should increase in size (once). */
	add_vals(ht, val, NUM_VALS);
	ok1(ht->bits == HTABLE_BASE_BITS + 1);
	ok1(ht->max < (1 << ht->bits));

	/* Mask should be set. */
	ok1(ht->common_mask != 0);
	ok1(ht->common_mask != -1);
	ok1(check_mask(ht, val, NUM_VALS));

	/* Find all. */
	find_vals(ht, val, NUM_VALS);
	ok1(!htable_get(ht, hash(&dne, NULL), objcmp, &dne));

	/* Walk once, should get them all. */
	i = 0;
	for (p = htable_first(ht,&iter); p; p = htable_next(ht, &iter))
		i++;
	ok1(i == NUM_VALS);

	/* Delete all. */
	del_vals(ht, val, NUM_VALS);
	ok1(!htable_get(ht, hash(&val[0], NULL), objcmp, &val[0]));

	/* Worst case, a "pointer" which doesn't have any matching bits. */
	htable_add(ht, 0, (void *)~(uintptr_t)&val[NUM_VALS-1]);
	htable_add(ht, hash(&val[NUM_VALS-1], NULL), &val[NUM_VALS-1]);
	ok1(ht->common_mask == 0);
	ok1(ht->common_bits == 0);

	/* Add the rest. */
	add_vals(ht, val, NUM_VALS-1);

	/* Check we can find them all. */
	find_vals(ht, val, NUM_VALS);
	ok1(!htable_get(ht, hash(&dne, NULL), objcmp, &dne));

	return exit_status();
}

#include "embedded_assets.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define lain_mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define lain_mkdir(path) mkdir(path, 0777)
#endif

#include "settings.h"

/* Provided by the generated lain_assets.S (.incbin of the packed assets). */
extern const unsigned char lain_assets_blob[];
extern const unsigned char lain_assets_blob_end[];

enum {
    EXTRACT_ALL,
    EXTRACT_CORE, /* fonts/, the launcher artwork, and only the pack names */
    EXTRACT_LANG  /* one subtitle pack */
};

static unsigned read_u32(const unsigned char **p)
{
    const unsigned char *b = *p;
    unsigned v = (unsigned)b[0] | ((unsigned)b[1] << 8) | ((unsigned)b[2] << 16) | ((unsigned)b[3] << 24);
    *p = b + 4;
    return v;
}

/* Creates every directory of `path` (the last component is the file). */
static void make_dirs(const char *path)
{
    char tmp[1200];
    size_t n = strlen(path);
    if (n >= sizeof tmp)
        return;
    memcpy(tmp, path, n + 1);
    for (size_t i = 1; i < n; i++)
    {
        if (tmp[i] == '/' || tmp[i] == '\\')
        {
            const char c = tmp[i];
            tmp[i] = 0;
            lain_mkdir(tmp);
            tmp[i] = c;
        }
    }
}

static int file_size_matches(const char *path, unsigned size)
{
    FILE *f = fopen(path, "rb");
    long n;
    if (!f)
        return 0;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fclose(f);
    return n == (long)size;
}

static int write_file(const char *path, const unsigned char *data, unsigned size)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return 0;
    if (size && fwrite(data, 1, size, f) != size)
    {
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}

static unsigned fnv1a(unsigned h, const unsigned char *p, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

static int wants(const char *rel, int mode, const char *code)
{
    if (mode == EXTRACT_ALL)
        return 1;
    if (mode == EXTRACT_CORE)
    {
        if (strncmp(rel, "fonts/", 6) == 0)
            return 1;
        if (strcmp(rel, "lain_logo.rgba") == 0)
            return 1;
        /* Just the label, so the language pickers can list the packs. */
        {
            const size_t n = strlen(rel);
            return strncmp(rel, "lang/", 5) == 0 && n > 9 && strcmp(rel + n - 9, "/pack.txt") == 0;
        }
    }
    if (!code || !*code)
        return 0;
    {
        char prefix[96];
        snprintf(prefix, sizeof prefix, "lang/%s/", code);
        return strncmp(rel, prefix, strlen(prefix)) == 0;
    }
}

static int extract(int mode, const char *code)
{
    const unsigned char *p = lain_assets_blob;
    const unsigned char *end = lain_assets_blob_end;
    const size_t total = (size_t)(end - p);
    char marker[1200];
    FILE *m;
    unsigned count;
    int written = 0;
    int failed = 0;
    unsigned h = 2166136261u;

    if (total < 12 || memcmp(p, "LAINAS01", 8) != 0)
        return 0;

    /* One marker per extraction of this asset build. */
    h = fnv1a(h, (const unsigned char *)&total, sizeof total);
    h = fnv1a(h, p, total < 4096 ? total : 4096);
    if (total > 4096)
        h = fnv1a(h, end - 4096, 4096);
    if (mode == EXTRACT_CORE)
        snprintf(marker, sizeof marker, "%s.assets-core-%08x", settings_data_dir(), h);
    else if (mode == EXTRACT_LANG)
        snprintf(marker, sizeof marker, "%s.assets-lang-%s-%08x", settings_data_dir(), code, h);
    else
        snprintf(marker, sizeof marker, "%s.assets-%08x", settings_data_dir(), h);

    m = fopen(marker, "rb");
    if (m)
    {
        fclose(m);
        return 0;
    }

    p += 8;
    count = read_u32(&p);
    for (unsigned i = 0; i < count; i++)
    {
        char rel[1024];
        char path[1200];
        unsigned plen, size;
        if (p + 4 > end)
            break;
        plen = read_u32(&p);
        if (plen + 4 > (size_t)(end - p) || plen >= sizeof rel)
            break;
        memcpy(rel, p, plen);
        rel[plen] = 0;
        p += plen;
        if (p + 4 > end)
            break;
        size = read_u32(&p);
        if (size > (size_t)(end - p))
            break;
        if (wants(rel, mode, code))
        {
            snprintf(path, sizeof path, "%s%s", settings_data_dir(), rel);
            make_dirs(path);
            if (!file_size_matches(path, size))
            {
                if (write_file(path, p, size))
                    written++;
                else
                    failed++;
            }
        }
        p += size;
    }

    /* Only mark it done when nothing failed, so a full SD card can be retried. */
    if (failed == 0)
    {
        m = fopen(marker, "wb");
        if (m)
        {
            fputs("ok\n", m);
            fclose(m);
        }
    }
    return written;
}

int lain_assets_extract_core(void)
{
    return extract(EXTRACT_CORE, NULL);
}

int lain_assets_extract_lang(const char *code)
{
    return extract(EXTRACT_LANG, code);
}

int lain_assets_extract(void)
{
    return extract(EXTRACT_ALL, NULL);
}

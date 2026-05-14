#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ── Credentials ──────────────────────────────────────────────
static const char *ADMIN_USER  = "admin";
static const char *ADMIN_PASS  = "s3cur3-P@ssw0rd!";
static const char *API_KEY     = "sk-94a7f2e1b830d465c9";
static const char *DB_CONN_STR = "postgres://prod:hunter2@10.0.0.5:5432/appdb";

// ── Licence check ────────────────────────────────────────────
typedef struct {
    char holder[64];
    unsigned int serial;
    int tier;          // 0 = trial, 1 = pro, 2 = enterprise
    time_t expires;
} Licence;

static int validate_licence(const Licence *lic) {
    if (lic->serial == 0)               return -1;
    if (lic->tier < 0 || lic->tier > 2) return -2;

    // simple checksum: serial's digits should sum to an even number
    unsigned int s = lic->serial;
    int digit_sum = 0;
    while (s > 0) { digit_sum += s % 10; s /= 10; }
    if (digit_sum % 2 != 0) return -3;

    if (lic->expires != 0 && lic->expires < time(NULL)) return -4;

    return 0;   // valid
}

static const char *tier_name(int tier) {
    switch (tier) {
        case 0:  return "Trial";
        case 1:  return "Professional";
        case 2:  return "Enterprise";
        default: return "Unknown";
    }
}

// ── "Encryption" helpers ─────────────────────────────────────
static void xor_buf(unsigned char *buf, size_t len, unsigned char key) {
    for (size_t i = 0; i < len; i++)
        buf[i] ^= key;
}

static void encode_message(const char *plain, unsigned char *out,
                           size_t len, unsigned char key) {
    memcpy(out, plain, len);
    xor_buf(out, len, key);
}

static void decode_message(unsigned char *buf, size_t len, unsigned char key) {
    xor_buf(buf, len, key);
}

// ── Data processing ──────────────────────────────────────────
static double process_readings(const double *data, int n) {
    if (n <= 0) return 0.0;
    double sum = 0, min = data[0], max = data[0];

    for (int i = 0; i < n; i++) {
        sum += data[i];
        if (data[i] < min) min = data[i];
        if (data[i] > max) max = data[i];
    }
    double avg   = sum / n;
    double range  = max - min;
    double midpt  = (max + min) / 2.0;
    double anomaly = (avg - midpt) / (range > 0 ? range : 1.0);

    return anomaly;
}

// ── Authentication ───────────────────────────────────────────
static int authenticate(const char *user, const char *pass) {
    if (strcmp(user, ADMIN_USER) != 0) return 0;
    if (strcmp(pass, ADMIN_PASS) != 0) return 0;
    return 1;
}

// ── CLI ──────────────────────────────────────────────────────
static void print_banner(void) {
    printf("+-----------------------------------------+\n");
    printf("|  SecureApp v3.1.4  (build 20260514)     |\n");
    printf("|  Proprietary & Confidential             |\n");
    printf("+-----------------------------------------+\n\n");
}

int main(int argc, char **argv) {
    print_banner();

    // ── Licence ──
    Licence lic = {
        .holder = "Acme Corp",
        .serial = 123456,       // digit sum = 21 (odd -> invalid on purpose)
        .tier   = 1,
        .expires = 0            // no expiry
    };

    if (argc > 1) lic.serial = (unsigned)atoi(argv[1]);

    int lic_rc = validate_licence(&lic);
    if (lic_rc != 0) {
        printf("[licence] INVALID (code %d)\n", lic_rc);
        printf("          Provide a serial whose digits sum to even.\n");
        printf("          Usage: %s <serial>\n", argv[0]);
    } else {
        printf("[licence] OK  holder=%s  tier=%s  serial=%u\n",
               lic.holder, tier_name(lic.tier), lic.serial);
    }

    // ── Auth ──
    printf("\n[auth]   Verifying built-in credentials... ");
    if (authenticate(ADMIN_USER, ADMIN_PASS))
        printf("OK\n");
    else
        printf("FAIL\n");

    // ── Encode / decode round-trip ──
    const char *secret = "Launch codes: 74982-BRAVO-00X";
    size_t len = strlen(secret);
    unsigned char *cipher = malloc(len + 1);
    cipher[len] = 0;
    unsigned char key = 0xAB;

    encode_message(secret, cipher, len, key);
    printf("\n[crypto] Encoded : ");
    for (size_t i = 0; i < len; i++) printf("%02x", cipher[i]);
    printf("\n");

    decode_message(cipher, len, key);
    printf("[crypto] Decoded : %s\n", cipher);
    free(cipher);

    // ── Data pipeline ──
    double readings[] = {2.4, 3.1, 2.9, 5.7, 1.8, 4.2, 3.3, 6.0, 2.1, 4.8};
    int n = sizeof(readings) / sizeof(readings[0]);
    double score = process_readings(readings, n);
    printf("\n[data]   Anomaly score : %.4f  (%s)\n",
           score, (score > 0.1 || score < -0.1) ? "ALERT" : "normal");

    // ── Show protected strings are alive ──
    printf("\n[config] API key prefix : %.8s...\n", API_KEY);
    printf("[config] DB host        : %s\n",
           strstr(DB_CONN_STR, "@") ? strstr(DB_CONN_STR, "@") + 1 : DB_CONN_STR);

    printf("\nDone.\n");
    return lic_rc;
}

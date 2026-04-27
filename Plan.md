# NAS File Ingestion Daemon — Project Foundation Document

**Project name:** `nas-ingestion-daemon`
**Language:** C (C11)
**Infrastructure:** Docker, Prometheus, Grafana, SQLite
**Server:** home-srv1 Linux
**Status:** Pre-development — Phase 1 not started

---

## 1. Honest Project Evaluation

### Why "image dedup daemon" undersells this

The original framing — an image dedup daemon — is technically accurate but
narratively weak. What this project actually is, scoped correctly, is a
**file ingestion pipeline daemon**: the central nervous system of the entire
home server NAS. Every file that enters the system passes through it. Every
other project depends on it.

The distinction matters for three reasons:

1. **CV impact** — "I built a file processing pipeline daemon in C that is
   the backbone of my home server infrastructure" is a fundamentally stronger
   statement than "I wrote a dedup script."

2. **Actual usefulness** — it processes ALL file types, not just photos.
   Documents are routed and trigger RAG indexing. Videos get thumbnails.
   Everything gets metadata. The daemon is useful from day one across the
   entire NAS.

3. **Technical depth** — a multi-file-type processing pipeline with a
   plugin-like handler architecture, a state machine per file type, and
   a proper event emission system is meaningfully more complex and
   interesting than a single-purpose script.

### Is this the best possible C project for this context?

Yes — for the following reasons:

- **C is the only sensible choice.** inotify, signal handling, POSIX file
  APIs, Unix socket IPC, and low-latency file processing are all areas where
  C is not just appropriate but correct. A recruiter asking "why C?" gets a
  genuine answer.

- **It is immediately and permanently useful.** From the moment Phase 1 is
  running, real files are being processed on the real NAS. This is not a
  toy project — it is infrastructure.

- **It feeds every other project.** The RAG pipeline needs docs routed to
  `/nas/docs`. The multi-modal search engine needs image metadata. The job
  scheduler will orchestrate the daemon. All four projects on the roadmap
  depend on this one.

- **The scope is right.** Large enough to demonstrate serious engineering.
  Small enough to finish. 800–1500 lines of well-written C is the sweet spot.

---

## 2. What The Daemon Actually Does

```
/nas/incoming/<device>/
        ↓
   [inotify event]
        ↓
   magic byte detection
   (NOT file extension)
        ↓
   ┌────┬────┬────┬────┐
   ↓    ↓    ↓    ↓    ↓
 IMAGE DOC  VIDEO PDF  UNKNOWN
   ↓    ↓    ↓    ↓    ↓
  [image pipeline]  [doc pipeline]  [quarantine]
   ↓
   xxHash64 exact dedup
   ↓
   pHash near-dedup (Hamming distance)
   ↓
   libvips quality score (Laplacian variance)
   ↓
   EXIF timestamp extraction
   ↓
   /nas/photos/YYYY/MM/DD/filename.jpg
   ↓
   metadata → SQLite
   ↓
   event emitted → Unix socket
   ↓
   Prometheus metrics updated
```

### File routing table

| File type      | Detection          | Destination         | Extra processing            |
|----------------|--------------------|---------------------|-----------------------------|
| JPEG/PNG/HEIC  | Magic bytes        | /nas/photos/YYYY/MM/DD | dedup, quality, EXIF    |
| PDF            | Magic bytes        | /nas/docs/          | page count metadata         |
| DOCX/TXT/MD    | Magic bytes + ext  | /nas/docs/          | word count metadata         |
| MP4/MOV        | Magic bytes        | /nas/photos/YYYY/MM/DD | duration, thumbnail     |
| Unknown        | —                  | /nas/incoming/quarantine/device/ | logged, manual review |
| Duplicate      | xxHash / pHash     | /nas/incoming/quarantine/device/ | reason logged        |
| Low quality    | Laplacian variance | /nas/incoming/quarantine/device/ | score logged         |

---

## 3. Architecture

### File structure

```
nas-ingestion-daemon/
├── src/
│   ├── main.c          # startup, config load, signal handlers, main loop
│   ├── watcher.c       # inotify recursive watcher, event loop
│   ├── detector.c      # magic byte file type detection
│   ├── pipeline.c      # dispatch to correct handler per file type
│   ├── image.c         # image pipeline: hash, pHash, quality, EXIF, route
│   ├── document.c      # document pipeline: route, metadata extract
│   ├── hasher.c        # xxHash64 + pHash + SQLite hash store
│   ├── quality.c       # libvips Laplacian variance quality scoring
│   ├── router.c        # file move/copy, folder creation, quarantine
│   ├── db.c            # SQLite interface, all queries, prepared statements
│   ├── metrics.c       # Prometheus exposition format HTTP endpoint
│   ├── health.c        # Unix socket health endpoint
│   ├── logger.c        # structured logging, log levels
│   └── config.c        # config file parsing
├── include/
│   ├── watcher.h
│   ├── detector.h
│   ├── pipeline.h
│   ├── image.h
│   ├── document.h
│   ├── hasher.h
│   ├── quality.h
│   ├── router.h
│   ├── db.h
│   ├── metrics.h
│   ├── health.h
│   ├── logger.h
│   └── config.h
├── tests/
│   ├── test_hasher.c
│   ├── test_detector.c
│   ├── test_quality.c
│   └── test_router.c
├── docker/
│   ├── Dockerfile
│   └── docker-compose.daemon.yml
├── config/
│   └── daemon.conf.example
├── docs/
│   └── architecture.md
├── CMakeLists.txt
├── .gitignore
└── README.md
```

### Configuration file (daemon.conf)

```ini
[paths]
watch_dir       = /nas/incoming
photos_dir      = /nas/photos
docs_dir        = /nas/docs
quarantine_dir  = /nas/incoming/quarantine

[image]
quality_threshold     = 100.0   # Laplacian variance — below this = blurry
phash_distance        = 8       # Hamming distance threshold for near-dedup

[daemon]
log_level             = INFO    # DEBUG, INFO, WARN, ERROR
metrics_port          = 9101    # Prometheus scrape port
health_socket         = /tmp/nas-daemon.sock

[db]
sqlite_path           = /var/lib/nas-daemon/hashes.db
```

### SQLite schema

```sql
-- Every file ever processed
CREATE TABLE files (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    filename    TEXT NOT NULL,
    device      TEXT NOT NULL,
    xxhash      TEXT,
    phash       TEXT,
    file_type   TEXT NOT NULL,
    action      TEXT NOT NULL,  -- 'accepted', 'duplicate', 'low_quality', 'quarantine'
    reason      TEXT,
    quality_score REAL,
    destination TEXT,
    processed_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- Hash store for dedup lookups
CREATE TABLE hashes (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    xxhash      TEXT UNIQUE NOT NULL,
    phash       TEXT,
    first_seen  DATETIME DEFAULT CURRENT_TIMESTAMP,
    file_id     INTEGER REFERENCES files(id)
);

CREATE INDEX idx_xxhash ON hashes(xxhash);
CREATE INDEX idx_phash  ON hashes(phash);
```

### Prometheus metrics exposed on :9101/metrics

```
# Files processed total, by type and action
nas_daemon_files_processed_total{type="image",action="accepted"} 1423
nas_daemon_files_processed_total{type="image",action="duplicate"} 89
nas_daemon_files_processed_total{type="image",action="low_quality"} 34
nas_daemon_files_processed_total{type="document",action="accepted"} 67

# Processing latency histogram (milliseconds)
nas_daemon_processing_latency_ms_bucket{le="10"} 1200
nas_daemon_processing_latency_ms_bucket{le="50"} 1389
nas_daemon_processing_latency_ms_bucket{le="100"} 1423

# Daemon health
nas_daemon_uptime_seconds 86400
nas_daemon_queue_depth 0
```

---

## 4. Build Phases

### Phase 1 — Foundation (Weekend 1, ~8h)

**Goal:** daemon starts, watches `/nas/incoming` recursively, logs every new
file, shuts down cleanly.

**What to build:**
- CMakeLists.txt linking all future dependencies
- `config.c` — parse `daemon.conf`, expose typed config struct
- `logger.c` — log levels, timestamps, structured output to stdout
- `watcher.c` — inotify recursive watcher, handle IN_CLOSE_WRITE events
  (not IN_CREATE — wait until file is fully written)
- `main.c` — startup, load config, SIGTERM graceful shutdown,
  SIGHUP config reload without restart

**Key technical decisions:**
- Use `IN_CLOSE_WRITE` not `IN_CREATE` — a file appears before it is
  fully written. Hashing an incomplete file is wrong.
- Recursive watching requires watching newly created subdirectories too —
  Syncthing may create device subfolders dynamically.
- SIGHUP must reload config atomically — swap config pointer, not
  partial update.

**Commit when:** daemon runs, prints every new file path, exits cleanly on
Ctrl+C, reloads config on `kill -HUP`.

---

### Phase 2 — File type detection + routing (Weekend 2, ~8h)

**Goal:** every file correctly identified by type and routed to the right
place.

**What to build:**
- `detector.c` — magic byte detection for JPEG, PNG, HEIC, PDF, DOCX,
  MP4, MOV. Read first 16 bytes, match against known signatures.
  Do NOT trust file extensions.
- `router.c` — move/copy files to destination, create date-based folder
  structure, handle quarantine with reason logging
- `pipeline.c` — dispatcher: call correct handler based on detected type
- `db.c` — SQLite init, prepared statements, insert file record

**Magic byte reference (implement these):**

| Type  | Offset | Bytes                        |
|-------|--------|------------------------------|
| JPEG  | 0      | FF D8 FF                     |
| PNG   | 0      | 89 50 4E 47 0D 0A 1A 0A      |
| PDF   | 0      | 25 50 44 46                  |
| MP4   | 4      | 66 74 79 70                  |
| HEIC  | 4      | 66 74 79 70 68 65 69 63      |

**Commit when:** drop a JPEG, PNG, PDF, and unknown file into incoming.
Each goes to the correct destination. Unknown goes to quarantine.
SQLite has a record of each.

---

### Phase 3 — Image pipeline: dedup + quality (Weekend 3, ~8h)

**Goal:** duplicates and bad quality images caught and quarantined.

**What to build:**
- `hasher.c` — xxHash64 exact dedup, pHash near-dedup with Hamming
  distance, SQLite lookup and insert
- `quality.c` — libvips Laplacian variance for sharpness scoring,
  configurable threshold
- `image.c` — full image pipeline: hash → phash → quality → EXIF →
  route to `/nas/photos/YYYY/MM/DD/`

**How pHash + Hamming distance works:**

Perceptual hash reduces an image to a 64-bit integer representing its
visual structure. Two images with a Hamming distance ≤ 8 are considered
near-duplicates (same photo, different compression or slight crop).

```c
// Hamming distance between two 64-bit hashes
int hamming_distance(uint64_t a, uint64_t b) {
    return __builtin_popcountll(a ^ b);
}
```

**How Laplacian variance works:**

Apply a Laplacian filter to the image (edge detection). Sharp images have
high variance in the result. Blurry images have low variance. Threshold is
configurable — start at 100.0 and tune based on your actual photos.

**EXIF fallback logic:**
1. Try libvips EXIF DateTimeOriginal
2. Fall back to EXIF DateTime
3. Fall back to file modification time
4. Use today's date as last resort

**Commit when:** send 3 copies of same photo — only one accepted. Send a
blurry photo — quarantined with score logged. Check SQLite for all records.

---

### Phase 4 — Production hardening (Weekend 4, ~8h)

**Goal:** daemon is production-grade — observable, health-checkable,
containerised, valgrind clean.

**What to build:**
- `metrics.c` — minimal HTTP server (raw sockets, no library) serving
  `/metrics` in Prometheus exposition format on port 9101
- `health.c` — Unix domain socket responding to ping with daemon status
  JSON: uptime, files processed, queue depth
- Multi-stage `Dockerfile` — build stage installs gcc + all libs,
  runtime stage copies binary + runtime libs only. Target < 60MB image.
- `docker-compose.daemon.yml` — integrates with existing monitoring stack,
  mounts `/nas` as volume, Prometheus scrapes `:9101/metrics`
- Grafana dashboard JSON — panels for files processed, rejection rate,
  processing latency, queue depth

**Valgrind discipline:**

Run this before considering any phase done:
```bash
valgrind --leak-check=full --error-exitcode=1 ./nas-daemon --test-mode
```

Zero leaks, zero errors before committing.

**Commit when:** `docker compose up` brings up the full stack. Grafana
shows daemon metrics alongside system metrics. `valgrind` is clean.

---

## 5. Key Technical Decisions Explained

These are the questions you must be able to answer in an interview:

**Why IN_CLOSE_WRITE not IN_CREATE?**
IN_CREATE fires the moment a file appears — it may still be being written
by Syncthing. Hashing or processing an incomplete file produces wrong
results. IN_CLOSE_WRITE fires when the writing process closes the file
descriptor, guaranteeing the file is complete.

**Why magic bytes not file extensions?**
Extensions are user-controlled and unreliable. A JPEG renamed to .txt
is still a JPEG. Reading the file header (magic bytes) tells you what
the file actually is regardless of name. This is how every production
file processing system works.

**Why xxHash not MD5 or SHA256?**
MD5 and SHA256 are cryptographic hashes designed for security, not speed.
xxHash64 is designed purely for speed — 20-30x faster than MD5 on modern
hardware with equivalent collision resistance for dedup purposes. You don't
need cryptographic security to detect duplicate photos.

**Why pHash in addition to xxHash?**
xxHash catches exact byte-for-byte duplicates. pHash catches perceptual
duplicates — same photo re-compressed by WhatsApp, screenshot of a photo,
slightly cropped version. Both are needed because the same image from
your camera roll may arrive via multiple paths in different formats.

**Why SQLite not Postgres?**
The daemon is a single process on a single machine. Postgres requires a
server process, a connection pool, and network overhead. SQLite is a file,
requires no server, and handles one writer perfectly. Right tool for the
right job — this is a good interview answer because it shows you chose
deliberately, not by default.

**Why a Unix socket for health?**
The job scheduler daemon (Project 2) runs on the same host. Unix sockets
are faster than TCP for local IPC, require no port allocation, and can
be permission-controlled via filesystem permissions. The job scheduler
does `connect()` to `/tmp/nas-daemon.sock` and sends a ping — no network
stack involved.

---

## 6. Good Practices To Apply Throughout

These are not optional — they are the difference between a project that
looks like a portfolio toy and one that looks like production code.

**Every return code checked.** No `fopen()` without checking for NULL.
No `write()` without checking the return value. C has no exceptions —
silent failures cascade into mysterious bugs.

**One logical change per commit.** "Add inotify watcher" is a commit.
"Add inotify watcher, fix config parser, add quality scoring" is three
commits you were too lazy to separate. Commit history is readable and
tells the story of the build.

**Meaningful commit messages.** Format:
```
feat: add inotify recursive watcher with IN_CLOSE_WRITE
fix: handle EINTR on inotify_read during signal delivery
refactor: extract hash store logic into db.c
```

**Header guards on every .h file:**
```c
#ifndef NAS_HASHER_H
#define NAS_HASHER_H
// ... declarations
#endif // NAS_HASHER_H
```

**No magic numbers.** Config file for thresholds. `#define` or `const`
for fixed values. No `if (score < 100.0)` in the middle of a function.

**Valgrind before every phase commit.** Memory leaks in C are silent and
accumulate. A daemon running for weeks will leak badly if not checked.

**README kept current.** How to build, how to configure, how to run,
what each file does. Write it as if explaining to yourself in six months.

---

## 7. How This Feeds The Other Projects

```
nas-ingestion-daemon
        │
        ├── /nas/photos/YYYY/MM/DD/    ──→  Project 4: Multi-modal search
        │   (organised, quality filtered)    (CLIP embeddings, pgvector)
        │
        ├── /nas/docs/                 ──→  Project 3: Temporal RAG
        │   (all documents routed here)      (chunked, embedded, indexed)
        │
        ├── Unix socket health         ──→  Project 2: Job scheduler
        │   (/tmp/nas-daemon.sock)           (health checks, orchestration)
        │
        └── Prometheus :9101/metrics   ──→  Existing Grafana stack
            (files, latency, errors)         (daemon dashboard)
```

The daemon is not one of four independent projects. It is the foundation
that makes the other three possible. This is the story to tell.

---

## 8. CV Talking Point

One paragraph covering everything:

> "Built a production file ingestion daemon in C — inotify-based recursive
> watcher on a Syncthing NAS, magic byte file type detection, exact
> deduplication via xxHash64, near-duplicate detection via perceptual
> hashing with Hamming distance thresholding, image quality scoring via
> libvips Laplacian variance, EXIF-based date folder organisation.
> Processes all file types: images routed to a date-organised photo
> library, documents routed to a NAS docs store that feeds a RAG pipeline.
> Exposes Prometheus metrics consumed by an existing Grafana stack with
> a custom dashboard. Runs in a multi-stage Docker container with graceful
> SIGTERM shutdown, SIGHUP config reload, and a Unix socket health endpoint.
> All processing state persisted in SQLite across restarts. Zero memory
> leaks under Valgrind."

---

## 9. Immediate Next Steps

In order, do not skip:

1. `sudo apt install build-essential cmake pkg-config git valgrind libvips-dev libxxhash-dev libsqlite3-dev -y`
2. Create GitHub repo `nas-ingestion-daemon`, clone to server
3. Set up SSH key on server for GitHub push
4. Create folder structure: `mkdir -p src include tests docker config docs`
5. Write `.gitignore` (C template from GitHub)
6. Write `CMakeLists.txt` skeleton
7. Write one-paragraph `README.md`
8. Write `daemon.conf.example`
9. Write `logger.c` and `logger.h` — first file, everything else uses it
10. Write `config.c` and `config.h` — second file, everything else reads config
11. Write `watcher.c` — inotify loop, prints filenames, handles SIGTERM
12. First real commit: `feat: add inotify recursive watcher`

**Do not move to Phase 2 until Phase 1 is committed and running on the server.**

---

*Document version: 1.0 — created April 2026*
*Server: home-srv1 | NAS: /nas | Monitoring: Grafana + Prometheus (Docker)*
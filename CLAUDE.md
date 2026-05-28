# CLAUDE.md — NAS Ingestion Daemon

## Mandatory Rule: Learnings File

**On EVERY prompt where the user asks a question and I explain something new or reference an important concept, I MUST append a concise entry to `MDs/LEARNINGS.md` immediately.**

This is non-negotiable. No exceptions. Every single prompt where learning happens gets a reference added — whether it's a correction, a new concept, or an explanation.

Format: short heading + code example or table + one-line explanation. Keep it small and referenceable.

---

## Project Info

- **Language:** C (C11)
- **Build:** CMake
- **Style:** snake_case for functions/fields, PascalCase for types, header guards on every .h
- **Commit style:** `feat:`, `fix:`, `refactor:` prefixes
- **Valgrind required** before every phase commit

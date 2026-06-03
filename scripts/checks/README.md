# Checks

These scripts are quick consistency checks for constants and prototype wiring.
They are not the main validation path.

Run the fast non-boot contract aggregate before booting:

- `make check-poly-contracts`

Prefer real boot tests:

- `make boot-poly-binfmt-arch-traps`
- `make boot-poly-call-arch-traps`
- `make boot-poly-full-arch-traps`

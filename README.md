# JSD/OS
A small operating system for 32 bit x86

## What does JSD/OS stand for

The actual meaning of the acronym is intentionally ambigous but might stand for one of "**J**ake **S**. **D**el Mastro **O**perating **S**ystem" or "**J**ake'**S** **D**isk **O**perating **S**ystem" or maybe even "**J**ake's **S**oftware **D**istribution **O**perating **S**ystem" or finally "**J**ava**S**cript is **D**isallowed **O**perating **S**ystem" 

## Goals

- [x] Run on all almost x86 hardware since the 386sx
- [x] Low Memory footprint < 1MB for a base system
- [x] Highly modular, don't use what you don't need
- [x] Mutitasking using CuFS
- [ ] Zero Screen Tearing by default
- [ ] SMP support
- [ ] AMD64 version

## Non Goals

- POSIX/Unix Compatibility
- Fair scheduling

## What's with "EPSILON" in the release names

The greek letter Epsilon is often used in mathematics to represent a very small positive value approaching zero, this represents the status of the OS at this point in time, not much, but more than nothing.

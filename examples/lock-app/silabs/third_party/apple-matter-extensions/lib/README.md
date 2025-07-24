# Apple Matter Extensions Standalone build / unit testing

Follow these instructions to building / test the Apple Matter Extensions library outside of a CHIP SDK application project.

Instructions for referencing the extensions library from a CHIP application project are available in the [top-level README](../README.md).

1. Create a symlink to your CHIP SDK working copy under `third_party/connectedhomeip`
1. Activate the CHIP pigweed environment if necessary
1. `gn gen out`
1. `ninja -C out tests_run`

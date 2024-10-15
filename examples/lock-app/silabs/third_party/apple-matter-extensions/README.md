# Apple Matter Extensions

Copyright (c) 2023 Apple Inc. All Rights Reserved.

Apple Matter Extensions is licensed under Apple Inc.'s MFi Sample Code License Agreement, which is
contained in the [LICENSE](LICENSE) file distributed with the Apple Matter Extensions, and only to
those who accept that license.

## Overview

This repository contains a collection of Apple extensions to the Matter specification and SDK.
Some of these are code examples that need to be copied into an application project manually,
while other functionality can be used simply by adding this repository as a third-party dependency.

## Using apple-matter-extensions as a dependency

Follow these steps to add an `apple-matter-extensions` dependency to your Matter SDK app project:

1. Locate the `third_party` directory. When building a standalone Matter app, this directory should
   already exist and should contain a `connectedhomeip` symlink or sub-directory for the Matter SDK.
1. Add a `apple-matter-extensions` symlink to this repository to the `third_party` directory.
   A git sub-module or a directory containing an unpacked archive of this repository work as well.
1. In the `BUILD.gn` file for your Matter app, add `"third_party/apple-matter-extensions"` to the
   `deps` of your application target.
1. The include paths for your target are automatically set up so that Apple extension functionality
   can be imported via the prefix `apple`

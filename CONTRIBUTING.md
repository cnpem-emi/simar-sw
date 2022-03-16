# Contributing
## Code style
### C
#### clang-tidy
A `clang-tidy` configuration file is made available at the root of the project. Please refer to it for coding styles and acceptable practices.

You can lint your code with `clang-tidy --config-file=.clang-tidy *.c`.

#### clang-format
We're using Chromium's style, with a maximum line length of 100 columns.

You can format your code with `clang-format -style=file -i file.c`.
### Python
Although few Python scripts are used (in the `python` folder), they follow the described flake8 and Black configs in the folder. Use `flake8 .` and `black`.

## Support
Maintain support for Debian >9, kernel >5.10 AM335X machines, other configurations are not supported.

![swege](https://sakhmatd.ru/assets/img/swege_banner.png)

swege is a Static WEbsite GEnerator written in C.

It leverages the [discount](http://www.pell.portland.or.us/~orc/Code/discount/)
library for generating a website from a set of Markdown files.

# FEATURES
* Under 300 lines of C (without counting external libraries).
* Incremental updates.
* Pretty fast!
* Almost no dependencies except for discount.
* Portable-ish (*nix at least).

# QUICK START
Clone this repository and enter it:

```
git clone https://github.com/sakhmatd/swege
cd swege
```

Compile and install swege:

```
make
sudo make install
```

Copy the example directory to a location of choice:

`cp -r example ~/mycoolsite`

Enter your directory, edit swege.ini and run swege:

```
cd ~/mycoolsite
$EDITOR swege.ini
swege
```

Your website will appear in a directory specified by the
configuration file.

# LICENSE
Copyright 2020 Sergei Akhmatdinov

Licensed under the Apache License, Version 2.0 (the "License");
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Contains parts of ini.h, licensed under the BSD 3-clause license.

# CREDITS
Many thanks to David Parsons for the Discount library.

# BUGS/TODO
* Better documentation.
* Windows support?

# CONTRIBUTING
Contributions are welcome no matter who you are and where you come from.

When submitting PRs, please maintain the [coding style](https://suckless.org/coding_style/)
used for the project.

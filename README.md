![TinyMUSE.png](https://belisariussmith.com/external/TinyMUSE.png "TinyMUSE.png")

# About TinyMUSE

TinyMUSE was created after the MUD codebase explosion of 1990 and was based on version 1.5 of TinyMUSH (itself based on TinyMUD). It had a fairly short lifespan, and was quickly surpassed in both code quality and features by its contemporaries. Though it was updated occasionally, it was for the most part a dead project, and the TinyMUSE server population was accordingly small compared to other MUSHes. Because the code as of the last major update (v1.9f3) was still *so outdated*, it is difficult to compile, *much less run* a TinyMUSE server.

For the sake of preservation &amp; posterity, this repository has been created to provide a compilable and working version of TinyMUSE. There is no intent to add modern _mud_ functionality nor extensibility, but simply to provide a working, stripped down, stable server so that someone can see how the software originally acted. To that end, some minor cleanup (such as removing support for older database versions), and improvements have been made, such as updating to new standards (and libraries) in the C language (e.g. varargs &rarr; stdargs).

This project is maintained by [Belisarius Smith](https://www.belisariussmith.com/ "Belisarius Smith").

## Attribution

- Jin (original author)

### v1.0 - v1.8a4
- Nils McCarthy (@shkoo, **nils@chezmoto.ai.mit.edu**)
- Erk (@Erk, **erk@chezmoto.ai.mit.edu**)
- Michael (@Bard **michael@chezmoto.ai.mit.edu**)

### v1.8a4 - v1.9f3
- Mark Eisenstat (@Morgoth **meisen@musenet.org**)
- Jason Hula (@Power_Shaper)
- Rick Harby (@Tabok **rharby@eznet.net**)
- Robert Peperkamp (@Redone **rpeperkamp@envirolink.org**) 

### v2.0
- Belisarius Smith (@Balr)

This project is a work of love, in appreciation for all the hours of fun from running (and playing) my first MUD server... to all those who wrote, maintained, and played, I thank you.

## Documentation

Comprehensive documentation for the TinyMUSE codebase:

- **[Software Architecture](docs/software-architecture.md)** - Overview of system design and components
- **[Server Architecture](docs/server-architecture.md)** - Technical implementation details  
- **[Features](docs/features.md)** - Complete list of application features
- **[Modernization Opportunities](docs/potential-issues.md)** - Areas for improvement and modernization

## Server Administration

See [ADMIN.md](ADMIN.md)

## Version

This is currently version 2.0 

## Original Changelog (pre-git)

See [CHANGES.md](CHANGES.md)

## Licensing

This project and its original code is licensed under multiple custom copyright licenses. See [COPYRIGHT.md](COPYRIGHT.md)

Primarily, attribution is what is mostly required for non-commercial purposes.

Any new code or separate code in version 2.0+ falls under the MIT license.

![MIT License](https://belisariussmith.com/external/mitlicense.png)

—


Commonly Used Logging Classes

Date:       11/20/2013
Author:     K. Loux
Copyright:  Kerry Loux 2013
Licence:    MIT (see LICENSE file)

This collection of classes are used in several of my projects, so I decided to break them out as a submodule for ease of maintenance.  This file contains some notes about using the classes contained in this repository.

There are several guides online for using git submodules.  Here is my summary of the most useful git commands.
- To add logging to your project as a submodule:
```
$ cd <root directory of your superproject>
$ git submodule add https://github.com/KerryL/logging.git
```

NOTE:  To add a submodule within another directory, the destination must be specified following the repository url, so instead of the last step above, it would be:
```
$ git submodule add https://github.com/KerryL/logging.git <desired path>/logging
```

- Cloning a repository using a submodule now requires an extra step:
````
$ git clone ...
$ cd <project directory created by above clone command>
$ git submodule update --init --recursive
````

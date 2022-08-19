# Allosphere App Tutorial

This tutorial shows how to build an application for the Allosphere (or similar
distributed systems). It assumes you already know tha basics of allolib, so you
might want to look at the primer tutorial first.

You can fully prototype applications for the Allosphere in desktop systems if
you plan ahead and handle the main issues that arise when moving the application
to the sphere:

 * State and parameter distribution
 * Filesystem access (for textures, audio files, etc.)
 * Building and deployment on the Allosphere cluster
 
This tutorial focuses on the first two aspects. Tutorials 1-4 show how to
use DistributedApp to synchronize data. There are two mechanisms. A synchronous
state distribution that is useful for large data, and an asynchronous method
using parameters that is easy to use and suitable for values that don't change
often, or while prototyping, when it might not be certain what data will be
distributed in the final application. You can test state and parameter
synchronization by launching two instances of the applicatrion on your desktop.
This will also test omni rendering to ensure that the renderers will draw
correctly.

Tutorial 5 shows techniques for managing file system access. it provides a few
common techniques to conditionally load data according to where the application
is running.

The final piece to build in the Allosphere is building, deploying and running.
The application must be built separately twice for the two different systems
currently in use: the MacOS audio machine and the Linux renderers. Clone the
repo on /Volumes/Data/your_name in the audio machine and in 
/alloshare/code/your_name from gr02. Gr02 is used as it has a slightly larger
set of tools installed thatn the other gr machines. After building, you can
then use a multishell to launch the application on the renderers.

You need to make sure you have copied the data to the dataRoot used in the
application. For large data, it is recommended that you copy it locally to each
renderer's data drive.


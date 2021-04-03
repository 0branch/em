# EM
This is the Editor for Mortals 'em' by George Coulouris.  An ancestor of 'ex/vi'.

# INFO
This contains the original fixed em code.

The goal is to merge as much as possible from the 'merges' branch into master.  Creating the definitive 'em'.

# CURRENT
Currently I have merged the useful parts of CUNY and KENT as well as one part of AUASM (as well as reimplementing standard UNIX signals from illumos) into master.  Other parts were mainly useful for timesharing (allowing the editing of files not owned by you, or setting chmod on files that were changed).  Since we don't do that anymore, it's pointless including it.

# LIMITATIONS
Even before my changes, the 'fixed' version would segfault when in open 'o' mode upon characters like '{' (when trying to write code).  My changes did not affect it as it is down to the getchr function, this only happens on Linux. BSD, SYSV, macOS are ok.
Feel free to do a pull request if you can fix it on Linux.

A good test for it is to try and write a basic C function.  If it doesn't crash, then it's fixed.

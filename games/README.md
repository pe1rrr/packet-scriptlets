directory structure:

`games
saves
`

Make sure you put the games in a directory that is by the user ID running the dfrotz emulator, I run the dfrotz under a unique user ID 'zorkuser' that has it's shell set to `rbash` (restricted bash) and `rbash` set up as the link describes below. 

dfrotz can write save files anywhere that is writable for the userid that dfrotz runs under from within a game- so be careful to implement this:

See https://veliovgroup.com/article/BmtWycSfZL37zXMZc/how-to-rbash

Example:
`
drwxr-xr-x 3 myuserid      myuserid         4096 Jan 31 19:32 games
drwxr-xr-x 2 zorkuser zorkuser    4096 Jan  5 09:07 saves 
`

Because my game files are owned by `myuserid` rather than the ID that dfrotz is running, dfrotz wont be able to overwrite game files with save files, if someone malicious tries to do that.

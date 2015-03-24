# Bugs #
The Issue Tracker is used for all bug reports. Even if you contacted the developers, you should still make sure there is a report on the Issue Tracker.

## Reporting ##
Create a new issue, leave the labels as defaults. Status should be `New` and there should be no owner. Make sure you select the right type for the bug report:
  * Most bugs should be `Type-Bug`, the default category.
  * For performance related issues ("lags on Radeon XYZ"), use `Type-Lag`.
Also, select the operating system on which the bug was experienced, if applicable.

**IMPORTANT**: Do NOT add your email address to the CC field. This is used to mean that you are collaborating on the issue. Instead, simply star the issue you want to be informed about and Google Code will let you know when anything changes.

## Fixing ##
Any developer wishing to work on the bug should fill in their username as owner and set the status to `Accepted`.

The issue can be closed if:
  * **The work is done.** Set the status to `Resolved` and ask the bug reporter to verify that the bug has been fixed. If it has, set the status to `Verified` otherwise make sure you change the status back to `Started`.
  * **There is no bug.** If the bug report is bogus, pull a Jedi mind trick and change the status to `Invalid`.
  * **Decided against it.** If the developers are in consensus that the issue should not be fixed as requested, change the status to `WontFix`.

# Development #
Before doing any work on the project make sure you follow these rules (or there will be trouble):
  1. Before working on anything, check the [Issue Tracker](http://code.google.com/p/plutocracy/issues/list) to see if someone is already working on it.
  1. If someone else is listed as owner in an Issue that covers your work, you cannot do any work on it! There are no exceptions!
  1. Always create a new Issue for any work that takes longer than a day or two.
  1. If you failed to create an Issue that covers your work, and someone has usurped your work idea, you cannot complain! It is your fault!

The issue tracker is primarily used to claim areas to work on. The workflow process is outlined below:

  1. **Discuss your idea.** Go to the team IRC channel (`#plutocracy` on [FreeNode](http://freenode.net/irc_servers.shtml)) and talk to everyone on the team about your new idea. If everyone agrees, you can proceed.
  1. **Create an issue on the tracker.** After discussing your new work idea with the team, you must create a new issue that states your username as owner. If you are collaborating with another developer, their name should be added to the CC field. Use appropriate labels:
    1. Status should be `Accepted`, or if you have started working `Started`.
    1. Select the correct type:
      * `Type-Asset` is for creating new or modifying existing graphic and sound files.
      * `Type-Docs` is for working on new or existing design docs and manuals.
      * `Type-Feature` is for adding new features to the program that involve code or script work.
      * `Type-Task` is used when nothing else fits.
    1. Select the correct priority:
      * `Priority-Critical` for issues that must be resolved before the release.
      * `Priority-Normal` for issues that are allowed to slip their deadline.
      * `Priority-Wishlist` for issues that would be nice but aren't necessary.
    1. Select the release for which the issue is targeted.
    1. Enter a descriptive summary.
    1. Briefly, in a starred list describe all the major points you will be working on.
    1. Save the issue before your browser crashes!
  1. **Report progress.** Keep everyone up-to-date. Post comments in your Issue and talk about your progress or let everyone in IRC know how your work is coming along.
  1. **Resolve the issue.** Your work on an issue can end in several ways:
    * **Success!** You're done! Good job, set the status to `Resolved` and ask another developer to verify that you didn't screw up. Once they have OK'd your work, either one of you can set the status of the issue to `Verified`.
    * **You don't have the time.** The work is good and should be done, but you specifically cannot do it because you don't have the time or know-how to complete the task. It's OK, it happens! Set the status to `Stalled` and inform the other developers. This lets everyone know that you have given up work and would like someone else to take over. Remove your name from the issue owner field.
    * **The idea doesn't work.** Sometimes only after working on an idea for a while do you realize that it just doesn't work. First, check with the other developers to make sure they can't think of a way to make it work or if someone else wants to take over. If no one can or will, set the status to `Failed`.

If you are not creating a new Issue to work on, you can still accept an existing one, open or stalled issue. Issues that have been marked with the green `Contribute` tag are ideal for volunteers.
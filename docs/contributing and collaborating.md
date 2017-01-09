# Contributing and collaborating
This project is an open source effort.  As such, anyone and everyone is welcome to participate.
However, a project without governance is not much of a project but conversly, a project with too
much governance is not fun to work upon.

The most important things I can say are as follows:

First the positives:

* If you think you don't have anything to contribute ... speak up ... everyone is welcome and
there is always something you can do to push the ball forward ... from writing user guides and
tests to deep internals.
* If something is confusing or you need assistance ... speak up ... if it isn't clear to you then the
chances are high that it won't be clear to anyone.  And that is itself an area that would then need
attention to make better.
* If you are feeling unhappy or slighted or not recognized ... speak up.  Let us know .. we can fix
that easily.

Now the negatives:

* If you are here for profit, self recognition or you don't play well with others ... thanks for your
interest but you won't be happy here.  This is a place for folks who would be friends with the goal of
pushing the project forward and nothing else.
* If you are of the nature of "its my way or no way" ... thanks for your interest but you won't be happy
here.  During the course of any project man has undertaken, decisions have to be made.  Ideally they turn
out to be the correct decisions (but not always) and sometimes they are subjective (we shall paint the wall
blue despite you wanting to paint it red).  ALL choices will be considered and constructive arguments and
discussions will be held ... but if a decision is made that goes against your own choice then ideally fall
in-line with that decision and we all move on to the next puzzle.

## Project standards
This project is way too new to have standards.  At this time they will only slow us down.  That said,
if you submit contributions don't be surprised if someone else comes along and "tidys" your code.
It doesn't mean that they are claiming "authorship" ... rather they are merely providing the project a service
in making it consistent and tidy for the future.

## Use of JS-Hint
For want of a better tool, we are using JS-Hint to validate that any JavaScript we write conforms to 
basic qualities.  The web site at:  http://jshint.com/ can be used as the target of a copy/paste of
source to parse the code.  For variables that are globals, we need to add a comment to our source
to indicate to JS-Hint that they are not undeclared but are instead defined elsewhere.  The format
of the comment is:

```
/* globals <name>, <name>, ..., <name> */
```
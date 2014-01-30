# Gravity

"I want a watchface with hands that change rotational velocity depending
on their position," my friend Charles said. "OK," I said. I'm fairly sure
we were both pretty drunk at the time. Chuck doesn't even own a Pebble.

The current implementation was my fourth take at trying to do it, and the
only one that ended up being feasible:

* Define a centre and radius of the watchface.
* Define a "virtual centre" a couple dozen pixels above the actual one.
* Determine the angle the watch hands would normally occupy using
  conventional means.
* Project a ray from the virtual centre at that angle. Where it intersects
  the radius of the actual centre is the edge point for the hand.
* We now have a triangle comprising actual centre, virtual centre and
  edge point. We know the angle of the v_centre point, and two side
  lengths (distance from actual to virtual centres is defined, and
  distance from actual centre to edge point is the watchface radius).
* We try really hard to remember high-school trigonometry, and use the
  law of sines to calculate the angle at the actual centre point.
  Technically this form of the law has two solutions, but by keeping
  the destance between virtual and actual centres less than the
  watchface radius we make sure there's only one solution and avoid
  horrible case-checking code.
* And finally triumphantly rotate the hand to that angle.

The Pebble firmware has lookup tables for sine and cosine baked in, but
nothing for arcsine and arccosine. So these are calculated on the fly
with routines that the Internet helpfully provided. To cut down on
possible battery drain, calculated values are cached - after an hour of
constant usage, the minute hand has calculated all possible angles and
future redraws for both hands are taken from the cache table.

Finally, there's an option to invert the foreground and background colours.
This code uses Pebble's JSKit configuration interface to call out to my
server. The web page I keep online is in the html directory.

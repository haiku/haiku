Layer class
###########

The Layer class is responsible for working with invalid regions and
serves as the shadow class for BViews.

Member Functions
================

Layer(BRect frame, const char \*name, int32 resize, int32 flags, ServerWindow \*win)
------------------------------------------------------------------------------------


1. acquire a new layer token
2. validate (and fix, if necessary) parameters and assign to member objects
3. initalize relation pointers to NULL
4. set level to -1
5. set invalid region to NULL
6. set full and visible regions to bounds
7. set child count to 0
8. get ServerWindow's window port and create a PortLink pointed at it


~Layer(void)
------------

1. if parent is non-NULL, call RemoveSelf()
2. if topchild is non-NULL, iterate through children and delete them.
3. delete invalid, full, and visible regions if non-NULL
4. delete the internal PortLink


void AddChild(Layer \*child, Layer \*before=NULL, bool rebuild=true)
--------------------------------------------------------------------

Function which corresponds to BView::AddChild


1) if child->parent is non-NULL, spew an error to stderr and return
2) set child->parent to this

   a) if before == NULL:

      A) if topchild is non-NULL, set child->uppersibling to child
      B) set child->lowersibling to topchild
      C) if topchild is NULL, set bottomchild to child
      D) set topchild to child

   b) if before != NULL:

      A) if before->upper is non-NULL, set child->upper to before->upper
      B) set before->upper to child
      C) set child->lower to before
      D) if child->upper is non-NULL, set child->upper->lower to child
      E) increment child's level

3) increment the child count
4) if rebuild flag is true, call RebuildRegions


RemoveChild(Layer \*child, bool rebuild=true)
---------------------------------------------

Function which corresponds to BView::RemoveChild


1. if child's parent != this, spew an error to stderr and return
2. set child's parent to NULL
3. set child's level to -1
4. if top child matches child, set top child to child's lower sibling
5. if bottom child matches child, set bottom child to child's upper sibilng
6. if child->uppersibling is non-NULL, set its lowersibling to the child's lowersibling
7. if child->lowersibling is non-NULL, set its uppersibling to the child's uppersibling
8. decrement child count
9. call RebuildRegions

void RemoveSelf(bool rebuild=true)
----------------------------------

Function which corresponds to BView::RemoveSelf


1. if parent is NULL, spew an error to stderr and return
2. call parent->RemoveChild(this)

void Invalidate(BRect r)
------------------------

void Invalidate(BRegion \*region)
---------------------------------


Marks the area passed to the function as needing to be redrawn

1) if parent is NULL, return
2) if the passed area intersects the layer's frame, add it to the
   invalid region, creating a new invalid object if necessary.
3) Iterate through all child layers, calling child->Invalidate() on the
   area converted from the parent

BRect Frame(void), BRect Bounds(void)
-------------------------------------

Frame() returns the layer's frame in its parent coordinates. Bounds()
returns the frame offset to (0,0).

void MoveBy(BPoint pt), void MoveBy(float x, float y)
-----------------------------------------------------

Moves the layer in its parent's coordinate space by the specified
amount

1. offset the frame by the specified amount
2. if parent is non-NULL, call parent->RebuildRegions()

void ResizeBy(BPoint pt), void ResizeBy(float x, float y)
---------------------------------------------------------

Resizes the layer by the specified amount and all children as
appropriate.

1) resize the frame by the specified amount
2) iterate through all children, checking the resize flags to see
   whether each should be resized and calling child->ResizeBy() by the
   appropriate amount if it is necessary.

int32 CountChildren(void)
-------------------------

Returns the number of children owned directly by the layer -
grandchildren not included and some assembly required. Instructions
are written in Yiddish.


bool IsDirty(void)
------------------

Returns true if the layer needs redrawn (if invalid region is
non-NULL).


BRect ConvertToTop(const BRect &r), BRegion ConvertToTop(const BRegion &r)
--------------------------------------------------------------------------


Converts the given data to the coordinates of the root layer in the
layer tree.


1) if parent is non-NULL, return the data.
2) if parent is NULL, call this: return (parent->ConvertToTop(
   data_offset_by_frame.left_top ) )

BRect ConvertFromTop(const BRect &r), BRegion ConvertFromTop(const BRegion &r)
------------------------------------------------------------------------------

Converts the given data from the coordinates of the root layer in the
layer tree.

1. if parent is non-NULL, return the layer's frame
2. if parent is NULL, call this: return (parent->ConvertFromTop(
   data_offset_by_frame.left_and_top \* -1 ) )

BRect ConvertToParent(const BRect &r), BRegion ConvertToParent(const BRegion &r)

Converts the given data to the coordinates of the parent layer

1. return the data offset by the layer's frame's top left point, i.e.
   frame.LeftTop()

BRect ConvertFromParent(const BRect &r), BRegion ConvertFromParent(const BRegion &r)
------------------------------------------------------------------------------------

Converts the given data from the coordinates of the parent layer

1. operates exactly like ConvertToParent, except that the offset
   values are multiplied by -1

void RebuildRegions(bool recursive=false)
-----------------------------------------

Rebuilds the visible and invalid layers based on the layer hierarchy.
Used to update the regions after a call to remove or add a child layer
is made or when a layer is hidden or shown.


1. get the frame
2. set full and visible regions to frame
3. iterate through each child and exclude its full region from the
   visible region if the child is visible.
4. iterate through each lowersibling and exclude its full region from
   the visible region if the it is visible and it intersects the layer's
   frame.

void MakeTopChild(void)
-----------------------

Makes the layer the top child owned by its parent. Note that the top
child is "behind" other children on the screen.


1. if parent is NULL, spew an error to stderr and return
2. if parent's top child equals this, return without doing anything
3. if lowersibling and uppersibling are both NULL, return without doing anything
4. save pointer to parent layer to a temporary variable
5. call RemoveSelf and then the former parent's AddChild

void MakeBottomChild(void)
--------------------------

Makes the layer the bottom child owned by its parent. Note that the
top child is "in front of" other children on the screen.

1. if parent is NULL, spew an error to stderr and return
2. if parent's bottom child equals this, return without doing anything
3. if lowersibling and uppersibling are both NULL, return without doing anything
4. save pointer to parent layer to a temporary variable
5. call RemoveSelf() with rebuild set to false
6. call former parent's AddChild (rebuild is false), setting the before
   parameter to the former parent's bottomchild
7. save lowersibling to a temporary variable
8. call lowersibling->RemoveSelf() with no rebuild
9. call the parent's AddChild() with the before set to this and rebuild set to true

void RequestDraw(const BRect &r)
--------------------------------

Requests that the layer be drawn on screen. The rectangle passed is in
the layer's own coordinates.

1. if invalid is NULL, return
2. set the PortLink opcode to B_DRAW
3. create a BMessage(B_DRAW) and attach all invalid rectangles to it
4. attach the view token to the message
5. flatten the message to a buffer, attach it to the PortLink, and Flush() it.
6. recurse through each child and call its RequestDraw() function if it
   intersects the child's frame

Layer \*FindLayer(int32 token)
------------------------------

Finds a child layer given an identifying token


1. iterate through children and check tokens. Return a match if found.
2. iterate through children, calling its FindLayer function, return any non-NULL results
3. return NULL - we got this far, so there is no match

Layer \*GetChildAt(BPoint pt, bool recursive=false)
---------------------------------------------------

Gets the child at a given point. if recursive is true, all layers
under the current one in the tree are searched. if the point is
contained by the current layer's frame and no child is found, this
function returns the current layer. if the point is outside the
current layer's frame, it returns NULL


1. if frame does not contain the point, return NULL

if recursive is true:

A. start at the \*bottom\* child and iterate upward.
B. if the child has children, call the child's GetChildAt with the point
   converted to the child's coordinate space, returning any non-NULL
   results
C. if the child is hidden, continue to the next iteration
D. if the child's frame contains the point, return the child
E. if none of the children contain the point, return this

if recursive is false:

A. start at the \*bottom\* child and iterate upward.
B. if the child is hidden, continue to the next iteration
C. if the child's frame contains the point, return the child
D. if none of the children contain the point, return this

PortLink \*GetLink(void)
------------------------

Returns the layer's internal PortLink object so a message can be sent
to the Layer's window.


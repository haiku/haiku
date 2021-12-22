Inspector Detector: the Shelf Inspector

Writing replicants is cool, and as our system evolves we'll see them used more often. Replicant creation is fairly easily, but there are some trouble spots. To help you avoid them I've started writing an application to smooth out the process: the Shelf Inspector. This app will work as a test bed for writing both replicants and containers. It will also show off some of the new scriptability found in all Replicants and BShelf objects. This is Part 1 of a multipart article.

The first step is to inspect (and manipulate) the contents of a container view. You can find the sample app associated with this text at <ftp://ftp.be.com/pub/samples/application_kit/ShelfInspector.zip>.

This app lets you interrogate a BShelf object (aka "container view") to see what replicants it contains. One common difficulty with replicants is managing the add-on/library that defines the replicant. The Shelf Inspector helps with this by showing the images currently loaded in the target application. It also lets you unload libraries that are no longer in use (be careful not to unload libraries that still are in use). This a good time to build and run the Shelf Inspector. You'll notice when you launch it that it also launches the Container demo application; these two apps work together.

The Target Shelf: popup in the Shelf Inspector window can target any one of three container views in the system. Try it! The top list shows currently loaded replicants; the lower list shows the libraries loaded by the target application. By selecting a replicant in the top list you can delete it (with the Delete button) or you can try copying (cloning) the replicant into the Container demo application.

A warning here: not all replicants want to be cloned this way, but I thought it was useful to show the possibility. Such cloning can screw up the Container demo. If you run into problems it might help to remove the backing store file for the Container demo
(delete  /boot/home/config/settings/Container_data).

That's about all there is to the app at this point. There will be more in the future. Now let's switch gears and talk about the implementation. The interesting code -- which figures out what replicants are loaded in some other application -- is in the following functions:

BMessenger	TInfoWindow::MessengerForTarget(type_code w)
int32		TInfoWindow::GetReplicantAt(int32 index)
status_t	TInfoWindow::GetReplicantName(int32 uid, BMessage *result)
status_t	TInfoWindow::DeleteReplicant(int32 uid)
status_t	TInfoWindow::ImportReplicant(int32 uid)

The MessengerForTarget function uses the BeOS scripting protocol to get a BMessenger for the target shelf in the target application. Taking the Deskbar as an example, the code looks like this:

	request.AddSpecifier("Messenger");
	request.AddSpecifier("Shelf");
	request.AddSpecifier("View", "Status");
	request.AddSpecifier("Window", "Deskbar");
	to = BMessenger("application/x-vnd.Be-TSKB", -1);

We're asking for the "Messenger" to the "Shelf" in the View "Status" in the Window "Deskbar" of the Deskbar application.


Every replicant living inside a BShelf object automatically inherits several "Properties":

   * ID - each replicant gets a unique ID when added to a shelf.
   * Name - the Name of the top view of the replicant.
   * Signature - the signature of the add-on defining the replicant.
   * Suites - the computer-readable description of these properties.
   * View - the property "pointing" to the top view of the replicant.

The GetReplicantName and GetReplicantAt functions use these properties to do their work. Looking more closely at GetReplicantAt, it asks the shelf for the ID of the replicant at a given index:

	BMessage	request(B_GET_PROPERTY);
	request.AddSpecifier("ID");			// want the ID
	request.AddSpecifier("Replicant", index); 	// the index
	// fTarget as returned by MessengerForTarget()
	fTarget.SendMessage(request, &reply);
	reply.FindInt32("result", &uid);

And that's how we get the replicant's ID, which remains valid across "saves." Thus, in the Container demo, IDs remain valid across quit/launch cycles. For shelves that don't save their state (e.g., the Deskbar), the ID for a replicant, such as the mailbox widget, potentially changes each time it is added to the shelf.

Just as every replicant supports a set of properties, so does a shelf. Every shelf defines one property called "Replicant" that supports the following actions:

    * counting the number of replicants
    * adding a new replicant
    * deleting an existing replicant
    * getting the "data archive" defining a replicant

ImportReplicant() and DeleteReplicant() use these features. ImportReplicant() gets a copy of the data archive (the archived BMessage) for the replicant. Then it sends that archive to the Container demo and asks it to create a new replicant. Let's take a closer look:

	BMessage	request(B_GET_PROPERTY); 		// Get the archive
	BMessage	uid_specifier(B_ID_SPECIFIER);

	// IDs are specified using code like so:
	uid_specifier.AddInt32("id", uid);
	uid_specifier.AddString("property", "Replicant");
	request.AddSpecifier(&uid_specifier);

	fTarget.SendMessage(&request, &reply);
	// various error checking omitted

	// OK, let's get the archive message
	BMessage	data;
	reply.FindMessage("result", &data);

	// get messenger to the shelf in the Container demo
	BMessenger	mess 	MessengerForTarget(CONTAINER_MESSENGER);
	BMessage	msg(B_CREATE_PROPERTY);
	request2.AddMessage("data", &data);
	return mess.SendMessage(&request2, &reply);


First we send a GET command, asking the shelf to return a copy of the archive data message for the replicant whose unique ID is uid. Then we extract the archived data message and add it to a CREATE message. This is sent to the shelf in the Container demo app, and that's it. Note that not all replicants respond well to this manipulation; the Shelf Inspector warns you if this is the case when you select the Copy to Container button. The Clock replicant works fine, so it's a good one to play with.

The Library list isn't all that useful right now, but it can let you see if you're properly using some of the advanced replicant features such as the be:load_each_time and be:unload_on_delete flags. If "unload on delete" is set in a replicant that is removed, the corresponding library should also be removed. If "load each time" is set and the same replicant is loaded multiple times you should also see the library loaded the same number of times. In a future version of the Shelf Inspector I'll include some configurable test replicants that show these features in action.

That's it for this installment. Have fun programming the BeOS!

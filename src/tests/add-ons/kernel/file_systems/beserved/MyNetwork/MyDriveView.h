#ifndef _MYDRIVEVIEW_H_
#define _MYDRIVEVIEW_H_

#include <Window.h>
#include <View.h>
#include <ScrollView.h>
#include <Bitmap.h>
#include <String.h>

#include <stdio.h>


class MyDriveView : public BView
{
	public:
		MyDriveView(BRect rect);
		~MyDriveView();

		void Draw(BRect rect);
		void SetDriveSpace(int percentFull, int files, int folders);

	private:
		BStatusBar *barFull;
		char msg[100];
};

#endif

#define ENTRY_HEIGHT 32

void setScroll(int newScroll)
{
	int scrollDelta = newScroll - scroll;
	scroll = newScroll;
}

int main()
{
	float scrollPos = 0.0f;
	float scrollDir = 0.0f
	const float scrollFriction = 0.9f;
	const float clampFriction = 0.9f;
	int  dragAnchor = 0;
	bool draging = false;
	int prevDragAnchor = 0;
	
	float maxScrollPos = 100;
	
	while (1)
	{
		scanKeys();
		
		// process input
		int down = keysDown();
		int up   = keysUp();
		
		touchPosition touchPos;
		touchRead(&touchPos);
		
		if (down & KEY_TOUCH)
		{
			// always stop when tapping the screen
			scrollDir = 0.0f;
			
			if (touchPos.px < SCREEN_WIDTH / 2)
			{
				prevDragAnchor = dragAnchour = touchPos.py;
				draging = true;
			}
			else
			{
				int canvasPos = touchPos.py - scrollPos;
				int entry = canvasPos / ENTRY_HEIGHT;
				selectEntry(entry);
			}
		}
		else if (up & KEY_TOUCH)
		{
			draging = false;
		}
		
		// animate
		if (!draging)
		{
			scrollPos += scrollDir;
			scrollDir *= scrollFriction;
			if (fabs(scrollDir) < 0.01f) scrollDir = 0.0f;
			
			if (scrollPos < 0.0f)
			{
				// clamp to start
				scrollDir = 0.0f;
				scrollPos *= clampFriction;
			}
			else if (scrollPos > maxScrollPos)
			{
				// clamp to end
				scrollDir = 0.0f;
				scrollPos = scrollPos + (maxScrollPos - scrollPos) * clampFriction;
			}
			
			setScroll(int(scrollPos));
		}
		else
		{
			setScroll(touchPos.py - dragAnchor);
			
			// track scrollDir
			scrollDir += touchPos.py - prevDragAnchor;
			scrollDir *= scrollFriction;
			prevDragAnchor = dragAnchor;
		}
		
		swiWaitForVBlank();
	}
}

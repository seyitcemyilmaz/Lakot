#ifndef LAKOT_GUI_H
#define LAKOT_GUI_H

class GUI {
private:
	bool mVisibility;

public:
	GUI();

	void initialize();

	void render();

	void setVisibility(bool pVisibility);
	bool getVisibility();
};

#endif

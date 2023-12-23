#ifndef LAKOT_SETTINGS_H
#define LAKOT_SETTINGS_H


class Settings {
private:
	static Settings* mInstance;

	bool mIsMouseMovementEnabled;

	Settings();
public:
	static Settings* getInstance();

	bool getIsMouseMovementEnabled();
	void setIsMouseMovementEnabled(bool pIsMouseMovementEnabled);

	Settings(Settings&) = delete;
	void operator=(const Settings&) = delete;
};

#endif

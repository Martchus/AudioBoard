#ifndef __CSSD1306_H__
#define __CSSD1306_H__

#define LCD_STYLE_LEFT		0x01
#define LCD_STYLE_RIGHT		0x02
#define LCD_STYLE_CENTER	0x03
#define LCD_STYLE_INVERT_T	0x04	// Text invertieren
#define LCD_STYLE_INVERT_L	0x08	// Zeile invertieren
#define LCD_STYLE_NOCLEAR	0x10	// Zeile vorm schreiben nicht löschen

#define LCD_GSTYLE_FILLED	0x01

class CSSD1306
{
//variables
public:
protected:
private:
	uint8_t data[1024];
	volatile uint8_t isr_num;
	uint8_t update_flags;
	volatile uint16_t data_pos;

//functions
public:
	CSSD1306();
	~CSSD1306();
	
	void init();
	void update();
	void clear();
	
	void print(uint8_t line, uint8_t style, const char *str, ...);
	void graph(uint8_t style, uint16_t *data, uint8_t num, uint16_t max);
	void graph2(uint16_t *data1, uint16_t *data2, uint8_t num, uint16_t max, uint8_t offset);
	
	uint8_t isBusy();
	
protected:
private:
	CSSD1306( const CSSD1306 &c );
	CSSD1306& operator=( const CSSD1306 &c );
	inline void Send(uint8_t c);
	inline void StartTransfer(uint8_t rw = 0);
	inline void EndTransfer();
	void SendCommand(uint8_t cmd);
	void SendData(uint8_t data);
	void SendData(uint8_t *data, uint16_t data_len);

}; //CSSD1306

#endif //__CSSD1306_H__

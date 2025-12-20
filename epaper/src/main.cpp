// 用于大连佳显的SPI电子纸屏和 Waveshare 开发板的显示库示例
// 需要硬件SPI和Adafruit_GFX库。注意：电子纸屏需要3.3V电源和数据线！
// 显示库基于佳显的演示示例：https://www.good-display.com/companyfile/32/
// 作者：Jean-Marc Zingg
// 版本：参见library.properties
// 库地址：https://github.com/ZinggJM/GxEPD2

// GxEPD2_WS_ESP32_Driver：适用于通用电子纸 raw 面板驱动板、ESP32 WiFi/蓝牙无线的GxEPD2_Example变体
// 参考链接：https://www.waveshare.com/product/e-paper-esp32-driver-board.htm

// Waveshare ESP32驱动板的引脚映射
// BUSY -> 25, RST -> 26, DC -> 27, CS-> 15, CLK -> 13, DIN -> 14

// 取消下一行注释以将HSPI用于EPD（VSPI用于SD），例如使用Waveshare ESP32驱动板
#define USE_HSPI_FOR_EPD

// 基类GxEPD2_GFX可用于传递显示实例的引用或指针作为参数，会多使用约1.2k代码
// 启用或禁用GxEPD2_GFX基类
#define ENABLE_GxEPD2_GFX 0

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
// #include <U8g2lib.h>
// 关键：U8g2适配Adafruit_GFX（GxEPD2继承自Adafruit_GFX）
#include "U8g2_for_Adafruit_GFX.h"
#include <cstdio>

// 选择显示类（仅一个），需与电子纸面板类型匹配
#define GxEPD2_DISPLAY_CLASS GxEPD2_BW

// 选择显示驱动类（仅一个），需与你的面板匹配
#define GxEPD2_DRIVER_CLASS GxEPD2_290     // GDEH029A1   128x296, SSD1608 (IL3820), (E029A01-FPC-A1 SYX1553)

// 定义显示类型相关的宏
#define GxEPD2_BW_IS_GxEPD2_BW true
#define IS_GxEPD(c, x) (c##x)
#define IS_GxEPD2_BW(x) IS_GxEPD(GxEPD2_BW_IS_, x)

#if defined(ESP32)
    #define MAX_DISPLAY_BUFFER_SIZE 65536ul // 最大显示缓冲区大小
    #if IS_GxEPD2_BW(GxEPD2_DISPLAY_CLASS)
    // 计算最大高度，确保不超过缓冲区大小
    #define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
    #endif

    // 定义显示类型
    typedef GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> DisplayType;

    // 初始化显示对象，参数为引脚：CS=15, DC=27, RST=26, BUSY=25
    DisplayType display(GxEPD2_DRIVER_CLASS(/*CS=*/ 15, /*DC=*/ 27, /*RST=*/ 26, /*BUSY=*/ 25));
#endif

// 部分刷新窗口配置（满足字节对齐）
#define REFRESH_X 8
#define REFRESH_Y 8
#define REFRESH_W 16
#define REFRESH_H 16
#define TEST_COUNT 20  // 测试次数（减少次数避免显示拥挤）


U8G2_FOR_ADAFRUIT_GFX u8g2gfx;

#include "epaper_bitmaps.h"

// 包含2.9英寸黑白屏的位图头文件
#include "bitmaps/Bitmaps128x296.h"

#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
// 定义HSPI对象
SPIClass hspi(HSPI);
#endif

// 声明需使用的字体（中文字库+英文字体，统一通过U8g2管理）
// 中文字库：u8g2_font_wqy16_t_gb2312b（16号文泉驿正黑，支持GB2312）
// 英文字体：u8g2_font_helvB12_tf（12号Helvetica粗体，与中文字体风格匹配）
const uint8_t* chineseFont = u8g2_font_wqy16_t_gb2312b;
const uint8_t* englishFont = u8g2_font_helvB12_tf;


void drawCustomContent();  // 绘制自定义内容
void helloWorld();
void helloWorldForDummies();
void helloFullScreenPartialMode();
void helloArduino();
void helloEpaper();
void deepSleepTest();
void showBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partial);
void drawCornerTest();
void showFont(const char name[], const GFXfont *f);
void drawFont(const char name[], const GFXfont *f);
void showPartialUpdate();
void drawBitmaps();
void drawBitmaps128x296();
void drawMyImage();
//统一文本显示函数（支持汉字、英文、数字混合显示）
void drawUniversalText(int16_t x, int16_t y, const char* text, const uint8_t* font, uint16_t color, uint8_t alignment = 0);
void testUnifiedTextDisplay();// 测试函数：验证统一接口的混合显示效果




void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");

  // *** 针对Waveshare ESP32驱动板的特殊处理 *** //
  // ********************************************************* //
  // *** 初始化HSPI，配置SPI模式和引脚 *** //
#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
  hspi.begin(14, -1, 13);  // sck=14, miso=-1(不使用), mosi=13(根据硬件连接配置)
  // 关键：使用SPI时，E029A01默认需要MODE0，若使用MODE3可能无法读取
  SPISettings epd_spi_settings(4000000, MSBFIRST, SPI_MODE0); // 4MHz速率，SPI模式为MODE0
  display.epd2.selectSPI(hspi, epd_spi_settings); // 选择配置好的SPI对象
#endif
  // *** 初始化显示屏（原函数display.init(115200)改为无参，保持默认配置）*** //
  display.init();
  display.setRotation(1);
  Serial.println("显示屏初始化完成"); // 打印提示确认初始化执行

  // 关键：初始化U8g2与GxEPD2显示对象的绑定
  u8g2gfx.begin(display);  // 将u8g2gfx与display关联，后续通过u8g2gfx绘图
//   drawCustomContent();  // 绘制自定义内容
//   delay(5000);
   // 显示自定义图片
//   display.setFullWindow(); // 全屏幕刷新（图片需要全刷新）
//   display.firstPage();     // 开始绘图循环
//   do {
//     display.fillScreen(GxEPD_WHITE); // 填充背景为白色
//     // 绘制自定义生成的图片（此处替换为实际图片对应的绘制函数）
//     draw_rgb_cam_1758806792_png();
//   } while (display.nextPage()); // 完成绘图并刷新屏幕
//     delay(5000);
//   display.setFullWindow();//diaplay.init()里面已经设置过了

  helloWorld();
  helloEpaper();


//   // 测试新的统一文本显示函数
//   testUnifiedTextDisplay();

    display.setPartialWindow(REFRESH_X, REFRESH_Y, REFRESH_W, REFRESH_H);
  unsigned long totalTime = 0;
  unsigned long minTime = 1000000;
  unsigned long maxTime = 0;

  for (int i = 0; i < TEST_COUNT; i++) {
    unsigned long start = micros();
    // 执行部分刷新（绘制简单内容）
    display.firstPage();
    do {
      display.fillRect(REFRESH_X, REFRESH_Y, REFRESH_W, REFRESH_H, i % 2 == 0 ? GxEPD_BLACK : GxEPD_WHITE);
    } while (display.nextPage());
    unsigned long end = micros();
    unsigned long duration = end - start;

    totalTime += duration;
    if (duration < minTime) minTime = duration;
    if (duration > maxTime) maxTime = duration;
    delayMicroseconds(100);  // 避免硬件过载
  }

  // 计算测试结果
  float avgDurationMs = totalTime / TEST_COUNT / 1000.0;
  float avgFps = 1000.0 / avgDurationMs;
  float maxFps = 1000000.0 / minTime;

  // 在屏幕上显示结果（全窗口刷新确保清晰）
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);  // 清空背景

    // 标题（居中）
    drawUniversalText(
      display.width() / 2, 20,
      "刷新率测试结果",
      chineseFont,
      GxEPD_BLACK,
      1  // 居中对齐
    );

    // 测试次数（左对齐）
    char countStr[32];
    sprintf(countStr, "测试次数：%d次", TEST_COUNT);
    drawUniversalText(
      10, 50,
      countStr,
      chineseFont,
      GxEPD_BLACK,
      0  // 左对齐
    );

    // 最小耗时
    char minStr[32];
    sprintf(minStr, "最小耗时：%luμs", minTime);
    drawUniversalText(
      10, 75,
      minStr,
      chineseFont,
      GxEPD_BLACK,
      0
    );

    // 最大耗时
    char maxStr[32];
    sprintf(maxStr, "最大耗时：%luμs", maxTime);
    drawUniversalText(
      10, 100,
      maxStr,
      chineseFont,
      GxEPD_BLACK,
      0
    );

    // 平均刷新率（右对齐）
    char avgFpsStr[32];
    sprintf(avgFpsStr, "平均：%.2f FPS", avgFps);
    drawUniversalText(
      display.width() - 10, 75,
      avgFpsStr,
      englishFont,
      GxEPD_BLACK,
      2  // 右对齐
    );

    // 最大刷新率（右对齐）
    char maxFpsStr[32];
    sprintf(maxFpsStr, "最大：%.2f FPS", maxFps);
    drawUniversalText(
      display.width() - 10, 100,
      maxFpsStr,
      englishFont,
      GxEPD_BLACK,
      2
    );

  } while (display.nextPage());


  display.powerOff();
  Serial.println("setup done");
}

void loop()
{
}

// 测试函数：验证统一接口的混合显示效果
void testUnifiedTextDisplay()
{
  display.setPartialFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);

    // 1. 左对齐：混合中英文+数字
    drawUniversalText(
      10, 14,
      "温度：25.5℃ 湿度：60%",  // 含汉字、数字、符号
      chineseFont,
      GxEPD_BLACK,
      0  // 左对齐
    );

    // 2. 居中对齐：英文句子
    drawUniversalText(
      display.width() / 2, 80,
      "ESP32 & E-Paper Demo",
      englishFont,
      GxEPD_BLACK,
      1  // 居中
    );

    // 3. 右对齐：中文句子
    drawUniversalText(
      display.width() - 10, 126,
      "统一接口测试成功",
      chineseFont,
      GxEPD_BLACK,
      2  // 右对齐
    );
  }
  while (display.nextPage());
  delay(5000);  // 显示5秒
}

/**
 * 统一文本显示函数（支持汉字、英文、数字混合显示）
 * @param x：文本左上角x坐标（或对齐基准点x）
 * @param y：文本基线y坐标（U8g2文本绘制以基线为基准，非左上角）
 * @param text：要显示的字符串（支持GB2312汉字+ASCII字符）
 * @param font：U8g2字体（如chineseFont/englishFont）
 * @param color：文本颜色（GxEPD_BLACK/GxEPD_WHITE，电子纸仅支持黑白）
 * @param alignment：对齐方式（0=左对齐，1=居中，2=右对齐）
 */
/**
 * 统一文本显示函数（支持汉字、英文、数字混合显示）
 * 基于字体特性优化：Ascent=14（基线到顶部距离），Descent=-2（基线到底部距离）
 * @param x：文本左上角x坐标（或对齐基准点x）
 * @param y：文本基线y坐标（U8g2文本绘制以基线为基准）
 * @param text：要显示的字符串（支持GB2312汉字+ASCII字符）
 * @param font：U8g2字体（如chineseFont/englishFont）
 * @param color：文本颜色（GxEPD_BLACK/GxEPD_WHITE）
 * @param alignment：对齐方式（0=左对齐，1=居中，2=右对齐）
 */
void drawUniversalText(int16_t x, int16_t y, const char* text, const uint8_t* font, uint16_t color, uint8_t alignment)
{
  // 1. 字体固定参数（根据测试得出）
  const int16_t ascent = 14;    // 基线到文本顶部的距离
  const int16_t descent = -2;   // 基线到文本底部的距离（负值）
  const uint16_t fontHeight = ascent - descent;  // 字体总高度：14 - (-2) = 16
  const uint16_t screenHeight = display.height(); // 屏幕高度（旋转后为128）

  // 2. 设置字体和颜色
  u8g2gfx.setFont(font);
  u8g2gfx.setForegroundColor(color == GxEPD_BLACK ? 0 : 1);
  u8g2gfx.setBackgroundColor(color == GxEPD_WHITE ? 0 : 1);

  // 3. 计算对齐后的x坐标
  if (alignment != 0)
  {
    uint16_t textWidth = u8g2gfx.getUTF8Width(text);
    if (alignment == 1)  // 居中对齐
    {
      x -= textWidth / 2;
    }
    else if (alignment == 2)  // 右对齐
    {
      x -= textWidth;
    }
  }

  // 4. 边界检查与y坐标调整（确保文本完全显示在屏幕内）
  int16_t textTop = y - ascent;       // 文本顶部坐标
  int16_t textBottom = y - descent;   // 文本底部坐标（实际为y + 2）

  // 若文本顶部超出屏幕上边界，调整到刚好可见
  if (textTop < 0)
  {
    y = ascent;  // 顶部对齐屏幕顶部（基线=ascent，此时textTop=0）
  }
  // 若文本底部超出屏幕下边界，调整到刚好可见
  else if (textBottom > screenHeight)
  {
    y = screenHeight + descent;  // 底部对齐屏幕底部（128 - 2 = 126）
  }

  // 5. 绘制文本（确保在调整后的安全坐标内）
  u8g2gfx.drawUTF8(x, y, text);
}


// void drawUniversalText(int16_t x, int16_t y, const char* text, const uint8_t* font, uint16_t color, uint8_t alignment)
// {
//   // 1. 设置字体
//   u8g2gfx.setFont(font);

//   // 2. 设置颜色（U8g2中1=前景色，0=背景色，需与GxEPD对应）
//   u8g2gfx.setForegroundColor(color == GxEPD_BLACK ? 0 : 1);
//   u8g2gfx.setBackgroundColor(color == GxEPD_WHITE ? 0 : 1);  // 背景色与屏幕一致

//   // 3. 计算对齐后的x坐标（根据对齐方式调整）
//   if (alignment != 0)
//   {
//     uint16_t textWidth = u8g2gfx.getUTF8Width(text);  // 获取文本宽度（U8g2内置方法，支持汉字）
//     if (alignment == 1)  // 居中对齐
//     {
//       x -= textWidth / 2;
//     }
//     else if (alignment == 2)  // 右对齐
//     {
//       x -= textWidth;
//     }
//   }

//   // 4. 绘制文本（U8g2的drawUTF8支持多编码字符）
//   u8g2gfx.drawUTF8(x, y, text);
// }


// 自定义内容绘制函数：文字 + 图形组合
void drawCustomContent() {
  // 1. 配置显示参数：旋转方向、字体、文字颜色
  display.setRotation(1);  // 屏幕旋转（0=默认，1=向右旋转90度，适配2.9英寸屏）
  display.setFont(&FreeMonoBold9pt7b);  // 设置字体（可替换为其他字体，如FreeSans12pt7b等）
  display.setTextColor(GxEPD_BLACK);    // 文字颜色（GxEPD_WHITE/GxEPD_BLACK，彩色屏可使用GxEPD_RED）

  // 2. 定义自定义数据：文字内容及屏幕尺寸
  const char* customText1 = "我的电子纸";    // 第一行文字
  const char* customText2 = "2025-12-16";   // 第二行文字（可替换为动态数据，如实时时间）
  uint16_t screenW = display.width();       // 屏幕宽度（2.9英寸屏旋转后为296px）
  uint16_t screenH = display.height();      // 屏幕高度（2.9英寸屏旋转后为128px）

  // 3. 开始绘图：使用firstPage() + do-while循环
  display.setFullWindow();  // 全屏幕刷新（若只需刷新部分区域，使用setPartialWindow()）
  display.firstPage();
  do {
    // 清空屏幕为白色（电子纸默认背景为白色，每次绘图前建议清空）
    display.fillScreen(GxEPD_WHITE);

    // --------------------------
    // 自定义内容1：显示居中文字
    // --------------------------
    // 计算文字1的边界框以实现居中
    int16_t tbx1, tby1; uint16_t tbw1, tbh1;
    display.getTextBounds(customText1, 0, 0, &tbx1, &tby1, &tbw1, &tbh1);
    uint16_t x1 = (screenW - tbw1) / 2 - tbx1;  // 水平居中
    uint16_t y1 = screenH / 3 - tby1;           // 垂直位置（1/3高度处）
    display.setCursor(x1, y1);
    display.print(customText1);

    // 计算文字2的边界框
    int16_t tbx2, tby2; uint16_t tbw2, tbh2;
    display.getTextBounds(customText2, 0, 0, &tbx2, &tby2, &tbw2, &tbh2);
    uint16_t x2 = (screenW - tbw2) / 2 - tbx2;
    uint16_t y2 = screenH * 2 / 3 - tby2;       // 垂直位置（2/3高度处）
    display.setCursor(x2, y2);
    display.print(customText2);

    // --------------------------
    // 自定义内容2：绘制图形（边框 + 圆 + 线）
    // --------------------------
    // 绘制屏幕边框（黑色，1px宽）
    display.drawRect(5, 5, screenW - 10, screenH - 10, GxEPD_BLACK);
    // 绘制中心填充圆（黑色，半径10px）
    display.fillCircle(screenW / 2, screenH / 2, 10, GxEPD_BLACK);
    // 绘制对角线（从左上角到右下角）
    display.drawLine(5, 5, screenW - 10, screenH - 10, GxEPD_BLACK);

  } while (display.nextPage());  // 完成绘图并刷新屏幕

  Serial.println("自定义内容显示完成");
}

// 注意：部分更新窗口和setPartialWindow()方法
// 部分更新窗口的大小和位置在物理x方向上是字节对齐的
// 对于偶数旋转，若x或w不是8的倍数，setPartialWindow()会增大尺寸；对于奇数旋转，y或h不是8的倍数时同理
// 详见GxEPD2_BW.h、GxEPD2_3C.h或GxEPD2_GFX.h中setPartialWindow()方法的注释

const char HelloWorld[] = "Hello World!";
const char HelloArduino[] = "Hello Arduino!";
const char HelloEpaper[] = "Hello E-Paper!";

// 示例：改造helloWorld函数
void helloWorld()
{
  display.setPartialFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);  // 清空背景

    // 显示英文（使用英文字体，居中对齐）
    drawUniversalText(
      display.width() / 2,    // x：屏幕中点（居中基准）
      display.height() / 2,   // y：基线位置（垂直居中）
      "Hello World!",         // 文本内容
      englishFont,            // 英文字体
      GxEPD_BLACK,            // 黑色文本
      1                       // 居中对齐
    );

    // 显示汉字（使用中文字库，在英文下方）
    drawUniversalText(
      display.width() / 2,
      display.height() / 2 + 30,  // 基线下移30px（字体高度约16px，留间距）
      "你好，世界！",
      chineseFont,
      GxEPD_BLACK,
      1
    );
  }
  while (display.nextPage());
}

void helloWorldForDummies()
{
  const char text[] = "Hello World!";
  // 大多数电子纸的原生方向是宽<高（竖屏），尤其是小尺寸屏幕
  // 在GxEPD2中，旋转0用于原生方向（大多数TFT库固定0为竖屏）
  // 旋转1（向右旋转90度）可在小屏幕上获得足够空间（横屏）
  display.setRotation(1);
  // 选择Adafruit_GFX中合适的字体
  display.setFont(&FreeMonoBold9pt7b);
  // 在电子纸上，黑底白字更易读
  display.setTextColor(GxEPD_BLACK);
  // Adafruit_GFX的getTextBounds()方法可确定当前字体下文本的边界框
  int16_t tbx, tby; uint16_t tbw, tbh; // 边界框窗口
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh); // 适用于原点(0,0)，tby可能为负
  // 通过原点转换使边界框居中
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  // 全窗口模式是初始模式，此处显式设置
  display.setFullWindow();
  // 此处使用分页绘图，即使处理器有足够RAM存储全缓冲区
  // 因此可用于任何支持的处理器板
  // 代码开销和执行时间损失很小
  // 告诉图形类使用分页绘图模式
  display.firstPage();
  do
  {
    // 这部分代码会执行多次（根据需要）
    // 若为全缓冲区，仅执行一次
    // 重要：每次迭代需绘制相同内容，避免异常效果
    // 使用可能变化的值的副本，不要在循环中读取模拟量或引脚！
    display.fillScreen(GxEPD_WHITE); // 将背景设为白色（填充缓冲区为白色值）
    display.setCursor(x, y); // 设置开始打印文本的位置
    display.print(text); // 打印文本
    // 多次执行部分结束
  }
  // 告诉图形类将缓冲区内容（页）传输到控制器缓冲区
  // 当最后一页传输完成，图形类会命令控制器刷新屏幕（对于无快速部分更新的面板）
  // 对于有快速部分更新的面板，当控制器缓冲区再次写入以使差分缓冲区相等时，返回false
  // （对于带快速部分更新的全缓冲，仅再次传输全缓冲区，返回false）
  while (display.nextPage());
}

void helloFullScreenPartialMode()
{
  const char fullscreen[] = "full screen update";
  const char fpm[] = "fast partial mode";
  const char spm[] = "slow partial mode";
  const char npm[] = "no partial mode";
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  const char* updatemode;
  if (display.epd2.hasFastPartialUpdate)
  {
    updatemode = fpm;
  }
  else if (display.epd2.hasPartialUpdate)
  {
    updatemode = spm;
  }
  else
  {
    updatemode = npm;
  }
  // 在循环外执行此操作
  int16_t tbx, tby; uint16_t tbw, tbh;
  // 居中更新文本
  display.getTextBounds(fullscreen, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t utx = ((display.width() - tbw) / 2) - tbx;
  uint16_t uty = ((display.height() / 4) - tbh / 2) - tby;
  // 居中更新模式
  display.getTextBounds(updatemode, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t umx = ((display.width() - tbw) / 2) - tbx;
  uint16_t umy = ((display.height() * 3 / 4) - tbh / 2) - tby;
  // 居中HelloWorld
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t hwx = ((display.width() - tbw) / 2) - tbx;
  uint16_t hwy = ((display.height() - tbh) / 2) - tby;
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(hwx, hwy);
    display.print(HelloWorld);
    display.setCursor(utx, uty);
    display.print(fullscreen);
    display.setCursor(umx, umy);
    display.print(updatemode);
  }
  while (display.nextPage());
}

void helloArduino()
{
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  // 与居中的HelloWorld对齐
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  // 高度可能不同
  display.getTextBounds(HelloArduino, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t y = ((display.height() / 4) - tbh / 2) - tby; // y是基线！
  // 使窗口足够大以覆盖（覆盖）先前文本的下行部分
  uint16_t wh = FreeMonoBold9pt7b.yAdvance;
  uint16_t wy = (display.height() / 4) - wh / 2;
  display.setPartialWindow(0, wy, display.width(), wh);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(HelloArduino);
  }
  while (display.nextPage());
  delay(1000);
}

void helloEpaper()
{
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  // 与居中的HelloWorld对齐
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  // 高度可能不同
  display.getTextBounds(HelloEpaper, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t y = (display.height() * 3 / 4) + tbh / 2; // y是基线！
  // 使窗口足够大以覆盖（覆盖）先前文本的下行部分
  uint16_t wh = FreeMonoBold9pt7b.yAdvance;
  uint16_t wy = (display.height() * 3 / 4) - wh / 2;
  display.setPartialWindow(0, wy, display.width(), wh);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(HelloEpaper);
  }
  while (display.nextPage());
}

void deepSleepTest()
{
  const char hibernating[] = "hibernating ...";
  const char wokeup[] = "woke up";
  const char from[] = "from deep sleep";
  const char again[] = "again";
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  // 居中文本
  display.getTextBounds(hibernating, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(hibernating);
  }
  while (display.nextPage());
  display.hibernate();
  delay(5000);
  display.getTextBounds(wokeup, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t wx = (display.width() - tbw) / 2;
  uint16_t wy = (display.height() / 3) + tbh / 2; // y是基线！
  display.getTextBounds(from, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t fx = (display.width() - tbw) / 2;
  uint16_t fy = (display.height() * 2 / 3) + tbh / 2; // y是基线！
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(wx, wy);
    display.print(wokeup);
    display.setCursor(fx, fy);
    display.print(from);
  }
  while (display.nextPage());
  delay(5000);
  display.getTextBounds(hibernating, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t hx = (display.width() - tbw) / 2;
  uint16_t hy = (display.height() / 3) + tbh / 2; // y是基线！
  display.getTextBounds(again, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t ax = (display.width() - tbw) / 2;
  uint16_t ay = (display.height() * 2 / 3) + tbh / 2; // y是基线！
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(hx, hy);
    display.print(hibernating);
    display.setCursor(ax, ay);
    display.print(again);
  }
  while (display.nextPage());
  display.hibernate();
}

void showBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partial)
{
  display.setRotation(1);
  if (partial)
  {
    display.setPartialWindow(x, y, w, h);
  }
  else
  {
    display.setFullWindow();
  }
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.fillRect(x, y, w, h, GxEPD_BLACK);
  }
  while (display.nextPage());
}

void drawCornerTest()
{
  display.setFullWindow();
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  for (uint16_t r = 0; r <= 4; r++)
  {
    display.setRotation(r);
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.fillRect(0, 0, 8, 8, GxEPD_BLACK);
      display.fillRect(display.width() - 18, 0, 16, 16, GxEPD_BLACK);
      display.fillRect(display.width() - 25, display.height() - 25, 24, 24, GxEPD_BLACK);
      display.fillRect(0, display.height() - 33, 32, 32, GxEPD_BLACK);
      display.setCursor(display.width() / 2, display.height() / 2);
      display.print(display.getRotation());
    }
    while (display.nextPage());
    delay(2000);
  }
}

void showFont(const char name[], const GFXfont* f)
{
  display.setFullWindow();
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();
  do
  {
    drawFont(name, f);
  }
  while (display.nextPage());
}

void drawFont(const char name[], const GFXfont* f)
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 0);
  display.println();
  display.println(name);
  display.println(" !\"#$%&'()*+,-./");
  display.println("0123456789:;<=>?");
  display.println("@ABCDEFGHIJKLMNO");
  display.println("PQRSTUVWXYZ[\\]^_");
  if (display.epd2.hasColor)
  {
    display.setTextColor(GxEPD_RED);
  }
  display.println("`abcdefghijklmno");
  display.println("pqrstuvwxyz{|}~ ");
}

// 注意：部分更新窗口和setPartialWindow()方法
// 部分更新窗口的大小和位置在物理x方向上是字节对齐的
// 对于偶数旋转，若x或w不是8的倍数，setPartialWindow()会增大尺寸；对于奇数旋转，y或h不是8的倍数时同理
// 详见GxEPD2_BW.h、GxEPD2_3C.h或GxEPD2_GFX.h中setPartialWindow()方法的注释
// showPartialUpdate()故意使用非8的倍数的值来测试此功能

void showPartialUpdate()
{
  // 一些有用的背景
  helloWorld();
  // 使用非对称值进行测试
  uint16_t box_x = 10;
  uint16_t box_y = 15;
  uint16_t box_w = 70;
  uint16_t box_h = 20;
  uint16_t cursor_y = box_y + box_h - 6;
  float value = 13.95;
  uint16_t incr = display.epd2.hasFastPartialUpdate ? 1 : 3;
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  // 显示更新框的位置
  for (uint16_t r = 0; r < 4; r++)
  {
    display.setRotation(r);
    display.setPartialWindow(box_x, box_y, box_w, box_h);
    display.firstPage();
    do
    {
      display.fillRect(box_x, box_y, box_w, box_h, GxEPD_BLACK);
    }
    while (display.nextPage());
    delay(2000);
    display.firstPage();
    do
    {
      display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
    }
    while (display.nextPage());
    delay(1000);
  }
  // 在更新框中显示更新内容
  for (uint16_t r = 0; r < 4; r++)
  {
    display.setRotation(r);
    display.setPartialWindow(box_x, box_y, box_w, box_h);
    for (uint16_t i = 1; i <= 10; i += incr)
    {
      display.firstPage();
      do
      {
        display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
        display.setCursor(box_x, cursor_y);
        display.print(value * i, 2);
      }
      while (display.nextPage());
      delay(500);
    }
    delay(1000);
    display.firstPage();
    do
    {
      display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
    }
    while (display.nextPage());
    delay(1000);
  }
}


void drawBitmaps()
{
  display.setFullWindow();
#ifdef _GxBitmaps128x296_H_
  drawBitmaps128x296();
#endif
}


#include <QObject>
#include <QColor>


class QColorMetaWrapper : public QObject
{
	Q_OBJECT

public:
	// QColorMetaWrapper(const QColor &color) : m_color(color) {}
	Q_INVOKABLE QColorMetaWrapper(const QString &name) : m_color(QColor(name)) {}
	Q_INVOKABLE QColorMetaWrapper(int color) : m_color(QColor(color)) {}
	Q_INVOKABLE QColorMetaWrapper(int r, int g, int b, int a = 255) : m_color(QColor(r, g, b, a)) {}
	Q_INVOKABLE QColorMetaWrapper () {}
	Q_INVOKABLE int alpha() const { return m_color.alpha(); }
	Q_INVOKABLE float alphaF() const { return m_color.alphaF(); }
	Q_INVOKABLE int black() const { return m_color.black(); }
	Q_INVOKABLE float blackF() const { return m_color.blackF(); }
	Q_INVOKABLE int	blue() const { return m_color.blue(); }
	Q_INVOKABLE float blueF() const { return m_color.blueF(); }
	Q_INVOKABLE int cyan() const { return m_color.cyan(); }
	Q_INVOKABLE float cyanF() const { return m_color.cyanF(); }
	// Q_INVOKABLE QColorMetaWrapper darker(int factor = 200) const { return QColorMetaWrapper(m_color.darker(factor)); }
	Q_INVOKABLE int	green() const { return m_color.green(); }
	Q_INVOKABLE float greenF() const { return m_color.greenF(); }
	Q_INVOKABLE int hslHue() const { return m_color.hslHue(); }
	Q_INVOKABLE float hslHueF() const { return m_color.hslHueF(); }
	Q_INVOKABLE int hslSaturation() const { return m_color.hslSaturation(); }
	Q_INVOKABLE float hslSaturationF() const { return m_color.hslSaturationF(); }
	Q_INVOKABLE int hsvHue() const { return m_color.hsvHue(); }
	Q_INVOKABLE float hsvHueF() const { return m_color.hsvHueF(); }
	Q_INVOKABLE int hsvSaturation() const { return m_color.hsvSaturation(); }
	Q_INVOKABLE float hsvSaturationF() const { return m_color.hsvSaturationF(); }
	Q_INVOKABLE int hue() const { return m_color.hue(); }
	Q_INVOKABLE float hueF() const { return m_color.hueF(); }
	Q_INVOKABLE bool isValid() const { return m_color.isValid(); }
	// Q_INVOKABLE QColorMetaWrapper lighter(int factor = 150) const { return QColorMetaWrapper(m_color.lighter(factor)); }
	Q_INVOKABLE int lightness() const { return m_color.lightness(); }
	Q_INVOKABLE float lightnessF() const { return m_color.lightnessF(); }
	Q_INVOKABLE int magenta() const { return m_color.magenta(); }
	Q_INVOKABLE float magentaF() const { return m_color.magentaF(); }
	Q_INVOKABLE QString name() const { return m_color.name(); }
	Q_INVOKABLE int red() const { return m_color.red(); }
	Q_INVOKABLE float redF() const { return m_color.redF(); }
	Q_INVOKABLE int rgb() const { return m_color.rgb(); }
	Q_INVOKABLE int rgba() const { return m_color.rgba(); }
	Q_INVOKABLE int saturation() const { return m_color.saturation(); }
	Q_INVOKABLE float saturationF() const { return m_color.saturationF(); }
	Q_INVOKABLE void setAlpha(int alpha) { return m_color.setAlpha(alpha); }
	Q_INVOKABLE void setAlphaF(float alpha) { m_color.setAlphaF(alpha); }
	Q_INVOKABLE void setBlue(int blue) { m_color.setBlue(blue); }
	Q_INVOKABLE void setBlueF(float blue) { m_color.setBlueF(blue); }
	Q_INVOKABLE void setCmyk(int c, int m, int y, int k, int a = 255) { m_color.setCmyk(c, m, y, k, a); }
	Q_INVOKABLE void setCmykF(float c, float m, float y, float k, float a = 1.0) { m_color.setCmykF(c, m, y, k, a); }
	Q_INVOKABLE void setGreen(int green) { m_color.setGreen(green); }
	Q_INVOKABLE void setGreenF(float green) { m_color.setGreenF(green); }
	Q_INVOKABLE void setHsl(int h, int s, int l, int a = 255) { m_color.setHsl(h, s, l, a); }
	Q_INVOKABLE void setHslF(float h, float s, float l, float a = 1.0) { m_color.setHslF(h, s, l, a); }
	Q_INVOKABLE void setHsv(int h, int s, int v, int a = 255) { m_color.setHsv(h, s, v, a); }
	Q_INVOKABLE void setHsvF(float h, float s, float v, float a = 1.0) { m_color.setHsvF(h, s, v, a); }
	Q_INVOKABLE void setNamedColor(const QString &name) { m_color.setNamedColor(name); }
	Q_INVOKABLE void setRed(int red) { m_color.setRed(red); }
	Q_INVOKABLE void setRedF(float red) { m_color.setRedF(red); }
	Q_INVOKABLE void setRgb(int r, int g, int b, int a = 255) { m_color.setRgb(r, g, b, a); }
	Q_INVOKABLE void setRgb(int rgb) { m_color.setRgb(rgb); }
	Q_INVOKABLE void setRgbF(float r, float g, float b, float a = 1.0) { m_color.setRgbF(r, g, b, a); }
	Q_INVOKABLE void setRgba(int rgba) { m_color.setRgba(rgba); }
	// Q_INVOKABLE QColor toCmyk() const
	// Q_INVOKABLE QColor toExtendedRgb() const
	// Q_INVOKABLE QColor toHsl() const
	// Q_INVOKABLE QColor toHsv() const
	// Q_INVOKABLE QColor toRgb() const
	Q_INVOKABLE int value() const { return m_color.value(); }
	Q_INVOKABLE float valueF() const { return m_color.valueF(); }
	Q_INVOKABLE int yellow() const { return m_color.yellow(); }
	Q_INVOKABLE float yellowF() const { return m_color.yellowF(); }


	// Q_INVOKABLE static QStringList colorNames() { return QColor::colorNames(); }
	// Q_INVOKABLE static QColorMetaWrapper fromCmyk(int c, int m, int y, int k, int a = 255) { return QColorMetaWrapper(QColor::fromCmyk(c, m, y, k, a)); }
	// Q_INVOKABLE static QColorMetaWrapper fromCmykF(qreal c, qreal m, qreal y, qreal k, qreal a = 1.0) { return QColorMetaWrapper(QColor::fromCmykF(c, m ,y, k, a)); }
	// Q_INVOKABLE static QColorMetaWrapper fromHsl(int h, int s, int l, int a = 255) { return QColorMetaWrapper(QColor::fromHsl(h, s, l, a)); }
	// Q_INVOKABLE static QColorMetaWrapper fromHslF(qreal h, qreal s, qreal l, qreal a = 1.0) { return QColorMetaWrapper(QColor::fromHslF(h, s, l, a)); }
	// Q_INVOKABLE static QColorMetaWrapper fromHsv(int h, int s, int v, int a = 255) { return QColorMetaWrapper(QColor::fromHsv(h, s, v, a)); }
	// Q_INVOKABLE static QColorMetaWrapper fromHsvF(qreal h, qreal s, qreal v, qreal a = 1.0) { return QColorMetaWrapper(QColor::fromHsvF(h, s, v, a)); }
	// Q_INVOKABLE static QColorMetaWrapper fromRgb(int rgb) { return QColorMetaWrapper(QColor::fromRgb(rgb)); }
	// Q_INVOKABLE static QColorMetaWrapper fromRgb(int r, int g, int b, int a = 255) { return QColorMetaWrapper(QColor::fromRgb(r, g, b, a)); }
	// Q_INVOKABLE static QColorMetaWrapper fromRgbF(qreal r, qreal g, qreal b, qreal a = 1.0) { return QColorMetaWrapper(QColor::fromRgbF(r, g, b, a)); }
	// Q_INVOKABLE static QColorMetaWrapper fromRgba(int rgba) { return QColorMetaWrapper(QColor::fromRgba(rgba)); }
	// Q_INVOKABLE static bool isValidColor(const QString &name) { return QColor::isValidColor(name); }
private:
	QColor m_color;
};

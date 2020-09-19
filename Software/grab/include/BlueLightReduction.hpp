#pragma once
#include <QList>
#include <QRgb>
#include <QRgba64>
#include <cassert>

namespace BlueLightReduction
{
	class Client
	{
	public:
		static bool isSupported() { assert(("BlueLightReduction::isSupported() is not implemented", false)); return false; }
		virtual void apply(QList<QRgb>& colors, const double gamma = 1.2) = 0;
		virtual void apply(QList<QRgba64>& colors, const double gamma = 1.2) = 0;
		virtual ~Client() = default;
	};

	Client* create();
};


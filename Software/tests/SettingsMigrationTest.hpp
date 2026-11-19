/*
 * SettingsMigrationTest.hpp
 *
 *	Project: Prismatik
 *
 *	Prismatik is a free, open-source software: you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as published
 *	by the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	Prismatik and Lightpack files is distributed in the hope that it will be
 *	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the GNU
 *	General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SETTINGSMIGRATIONTEST_HPP
#define SETTINGSMIGRATIONTEST_HPP

#include <QObject>

class SettingsMigrationTest : public QObject
{
	Q_OBJECT
public:
	SettingsMigrationTest() = default;

private Q_SLOTS:
	void testLegacyProfileCreatesBackupAndMigrates();
	void testIdempotentWhenAlreadyLogical();
	void testNewNonExistentProfileSkipsBackup();
};

#endif // SETTINGSMIGRATIONTEST_HPP

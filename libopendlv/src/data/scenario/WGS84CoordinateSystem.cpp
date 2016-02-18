/**
 * hesperia - Simulation environment
 * Copyright (C) 2008 - 2015 Christian Berger, Bernhard Rumpe
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <ostream>
#include <string>

#include "opendavinci/core/SharedPointer.h"
#include "opendavinci/core/base/Deserializer.h"
#include "opendavinci/core/base/Hash.h"
#include "opendavinci/core/base/SerializationFactory.h"
#include "opendavinci/core/base/Serializer.h"
#include "opendlv/data/scenario/CoordinateSystem.h"
#include "opendlv/data/scenario/ScenarioVisitor.h"
#include "opendlv/data/scenario/Vertex3.h"
#include "opendlv/data/scenario/WGS84CoordinateSystem.h"

namespace hesperia {
    namespace data {
        namespace scenario {

            using namespace std;
            using namespace core::base;

            WGS84CoordinateSystem::WGS84CoordinateSystem() :
                    m_origin() {}

            WGS84CoordinateSystem::WGS84CoordinateSystem(const WGS84CoordinateSystem &obj) :
                    CoordinateSystem(obj),
                    m_origin(obj.m_origin) {}

            WGS84CoordinateSystem::~WGS84CoordinateSystem() {}

            WGS84CoordinateSystem& WGS84CoordinateSystem::operator=(const WGS84CoordinateSystem &obj) {
                CoordinateSystem::operator=(obj);
                setOrigin(obj.getOrigin());
                return (*this);
            }

            void WGS84CoordinateSystem::accept(ScenarioVisitor &visitor) {
                visitor.visit(*this);

                m_origin.accept(visitor);
            }

            const Vertex3 WGS84CoordinateSystem::getOrigin() const {
                return m_origin;
            }

            void WGS84CoordinateSystem::setOrigin(const Vertex3 &o) {
                m_origin = o;
            }

            const string WGS84CoordinateSystem::toString() const {
                stringstream s;
                s << CoordinateSystem::toString() << ", " << m_origin.toString();
                return s.str();
            }

            ostream& WGS84CoordinateSystem::operator<<(ostream &out) const {
                // Serializer super class.
                CoordinateSystem::operator<<(out);

                SerializationFactory& sf=SerializationFactory::getInstance();

                core::SharedPointer<Serializer> s = sf.getSerializer(out);

                s->write(CRC32 < OPENDAVINCI_CORE_STRINGLITERAL6('o', 'r', 'i', 'g', 'i', 'n') >::RESULT,
                        m_origin);

                return out;
            }

            istream& WGS84CoordinateSystem::operator>>(istream &in) {
                // Deserializer super class.
                CoordinateSystem::operator>>(in);

                SerializationFactory& sf=SerializationFactory::getInstance();

                core::SharedPointer<Deserializer> d = sf.getDeserializer(in);

                d->read(CRC32 < OPENDAVINCI_CORE_STRINGLITERAL6('o', 'r', 'i', 'g', 'i', 'n') >::RESULT,
                       m_origin);

                return in;
            }

        }
    }
} // hesperia::data::scenario
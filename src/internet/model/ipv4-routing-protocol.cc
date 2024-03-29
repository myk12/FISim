/*
 * Copyright (c) 2009 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ipv4-routing-protocol.h"

#include "ipv4-route.h"

#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4RoutingProtocol");

NS_OBJECT_ENSURE_REGISTERED(Ipv4RoutingProtocol);

TypeId
Ipv4RoutingProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv4RoutingProtocol").SetParent<Object>().SetGroupName("Internet");
    return tid;
}

#ifdef FISIM_NAME_FIRST_ROUTING
std::unordered_map<uint64_t, Ipv4Address> Ipv4RoutingProtocol::m_name2addr;
#endif

} // namespace ns3

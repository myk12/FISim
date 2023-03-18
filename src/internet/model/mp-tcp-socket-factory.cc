#include "mp-tcp-socket-factory.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(MpTcpSocketFactory);

TypeId
MpTcpSocketFactory::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::MpTcpSocketFactory").SetParent<TcpSocketFactory>().SetGroupName("Internet");
    return tid;
}

} // namespace ns3

// ====================================================================== 
// \title  TlmPacketizer.hpp
// \author tcanham
// \brief  cpp file for TlmPacketizer test harness implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the Office
// of Technology Transfer at the California Institute of Technology.
// 
// This software may be subject to U.S. export control laws and
// regulations.  By accepting this document, the user agrees to comply
// with all U.S. export laws and regulations.  User has the
// responsibility to obtain export licenses, or other export authority
// as may be required before exporting such information to foreign
// countries or providing access to foreign persons.
// ====================================================================== 

#include "Tester.hpp"

#define INSTANCE 0
#define MAX_HISTORY_SIZE 10
#define QUEUE_DEPTH 10

#include <Fw/Com/ComPacket.hpp>

namespace Svc {

  // ----------------------------------------------------------------------
  // Construction and destruction 
  // ----------------------------------------------------------------------

  Tester ::
    Tester(void) : 
#if FW_OBJECT_NAMES == 1
      TlmPacketizerGTestBase("Tester", MAX_HISTORY_SIZE),
      component("TlmPacketizer")
#else
      TlmPacketizerGTestBase(MAX_HISTORY_SIZE),
      component()
#endif
      ,m_timeSent(false)
  {
    this->initComponents();
    this->connectPorts();
  }

  Tester ::
    ~Tester(void) 
  {
    
  }

  // ----------------------------------------------------------------------
  // Tests 
  // ----------------------------------------------------------------------

  // Some Test tables

  TlmPacketizerChannelEntry packet1List[] = {
          {10, 4},
          {100, 2},
          {333, 1}
  };

  TlmPacketizerChannelEntry packet2List[] = {
          {10, 4},
          {13, 8},
          {250, 2},
          {22,1}
  };

  TlmPacketizerPacket packet1 = {packet1List, 4, 1, FW_NUM_ARRAY_ELEMENTS(packet1List)};

  TlmPacketizerPacket packet2 = {packet2List, 8, 2, FW_NUM_ARRAY_ELEMENTS(packet2List)};

  TlmPacketizerPacketList packetList = {
              {&packet1, &packet2},
              2
          };

  TlmPacketizerChannelEntry ignoreList[] = {
          {25, 0},
          {50, 0}
  };

  TlmPacketizerPacket ignore = {ignoreList, 0, 0, FW_NUM_ARRAY_ELEMENTS(ignoreList)};

  void Tester ::
    initTest(void)
  {
      this->component.setPacketList(packetList,ignore,2);

  }

  void Tester ::
    pushTlmTest(void)
  {
      this->component.setPacketList(packetList,ignore,2);
      Fw::Time ts;
      Fw::TlmBuffer buff;

      // first channel
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U32)20));
      this->invoke_to_TlmRecv(0,10,ts,buff);

      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)15));
      this->invoke_to_TlmRecv(0,100,ts,buff);

      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)14));
      this->invoke_to_TlmRecv(0,333,ts,buff);

      // second channel
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U32)50));
      this->invoke_to_TlmRecv(0,10,ts,buff);

      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U64)1000000));
      this->invoke_to_TlmRecv(0,13,ts,buff);

      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)1010));
      this->invoke_to_TlmRecv(0,250,ts,buff);

      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)15));
      this->invoke_to_TlmRecv(0,22,ts,buff);

  }


  void Tester ::
    sendPacketsTest(void)
  {
      this->component.setPacketList(packetList,ignore,2);
      Fw::Time ts;
      Fw::TlmBuffer buff;

      // first channel
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U32)20));
      this->invoke_to_TlmRecv(0,10,ts,buff);

      // second channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)15));
      this->invoke_to_TlmRecv(0,100,ts,buff);

      // third channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)14));
      this->invoke_to_TlmRecv(0,333,ts,buff);

      // fifth channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U64)1000000));
      this->invoke_to_TlmRecv(0,13,ts,buff);

      // sixth channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)1010));
      this->invoke_to_TlmRecv(0,250,ts,buff);

      // seventh channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)15));
      this->invoke_to_TlmRecv(0,22,ts,buff);

      this->setTestTime(this->m_testTime);
      // run scheduler port to send packets
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      ASSERT_FROM_PORT_HISTORY_SIZE(2);
      ASSERT_from_PktSend_SIZE(2);

      // construct the packet buffers and make sure they are correct

      Fw::ComBuffer comBuff;
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(this->m_testTime));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)15));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)14));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(this->m_testTime));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)1000000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)1010));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)15));

      ASSERT_from_PktSend(1,comBuff,(U32)0);

  }

  void Tester ::
    sendPacketLevelsTest(void)
  {
      this->component.setPacketList(packetList,ignore,1);
      Fw::Time ts;
      Fw::TlmBuffer buff;

      // first channel
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U32)20));
      this->invoke_to_TlmRecv(0,10,ts,buff);

      // second channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)15));
      this->invoke_to_TlmRecv(0,100,ts,buff);

      // third channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)14));
      this->invoke_to_TlmRecv(0,333,ts,buff);

      // fifth channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U64)1000000));
      this->invoke_to_TlmRecv(0,13,ts,buff);

      // sixth channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)1010));
      this->invoke_to_TlmRecv(0,250,ts,buff);

      // seventh channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)15));
      this->invoke_to_TlmRecv(0,22,ts,buff);

      this->setTestTime(this->m_testTime);
      // run scheduler port to send packets
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      ASSERT_FROM_PORT_HISTORY_SIZE(2);
      ASSERT_from_PktSend_SIZE(2);

      // construct the packet buffers and make sure they are correct

      Fw::ComBuffer comBuff;
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(this->m_testTime));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)15));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)14));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(this->m_testTime));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)1000000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)1010));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)15));

      ASSERT_from_PktSend(1,comBuff,(U32)0);

  }

  void Tester ::
    updatePacketsTest(void)
  {
      this->component.setPacketList(packetList,ignore,2);
      Fw::Time ts;
      Fw::TlmBuffer buff;

      Fw::ComBuffer comBuff;

      // Initially no packets should be pushed
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      // Should be no packets pushed
      ASSERT_from_PktSend_SIZE(0);

      // first channel
      ts.set(100,1000);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U32)20));
      this->invoke_to_TlmRecv(0,10,ts,buff);

      this->m_testTime.add(1,0);
      this->setTestTime(this->m_testTime);
      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(2);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)0));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)0));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)0));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)0));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)0));

      ASSERT_from_PktSend(1,comBuff,(U32)0);

      // second channel

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)15));
      this->invoke_to_TlmRecv(0,100,ts,buff);

      this->m_testTime.add(1,0);
      this->setTestTime(this->m_testTime);
      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      // only one should be pushed
      ASSERT_from_PktSend_SIZE(1);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)15));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)0));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)14));
      this->invoke_to_TlmRecv(0,333,ts,buff);

      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      // only one should be pushed
      ASSERT_from_PktSend_SIZE(1);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)15));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)14));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U64)1000000));
      this->invoke_to_TlmRecv(0,13,ts,buff);

      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(1);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)1000000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)0));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)0));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)1010));
      this->invoke_to_TlmRecv(0,250,ts,buff);

      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(1);

      comBuff.resetSer();

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)1000000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)1010));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)0));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)15));
      this->invoke_to_TlmRecv(0,22,ts,buff);

      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(1);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)1000000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)1010));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)15));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      //** Update all the packets again with new values

      // first channel
      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U32)1000));
      this->invoke_to_TlmRecv(0,10,ts,buff);

      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(2);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)1000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)15));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)14));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)1000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)1000000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)1010));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)15));

      ASSERT_from_PktSend(1,comBuff,(U32)0);

      // second channel

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)550));
      this->invoke_to_TlmRecv(0,100,ts,buff);

      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(1);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)1000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)550));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)14));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)211));
      this->invoke_to_TlmRecv(0,333,ts,buff);

      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(1);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)1000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)550));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)211));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U64)34441));
      this->invoke_to_TlmRecv(0,13,ts,buff);

      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(1);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)1000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)34441));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)1010));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)15));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)8649));
      this->invoke_to_TlmRecv(0,250,ts,buff);

      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(1);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)1000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)34441));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)8649));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)15));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)65));
      this->invoke_to_TlmRecv(0,22,ts,buff);

      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(1);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)1000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)34441));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)8649));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)65));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

  }

  void Tester ::
     ignoreTest(void)
  {
      this->component.setPacketList(packetList,ignore,2);
      Fw::Time ts;
      Fw::TlmBuffer buff;

      Fw::ComBuffer comBuff;

      // Initially no packets should be pushed
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      // Should be no packets pushed
      ASSERT_from_PktSend_SIZE(0);

      // first channel
      ts.set(100,1000);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U32)20));
      this->invoke_to_TlmRecv(0,10,ts,buff);

      this->m_testTime.add(1,0);
      this->setTestTime(this->m_testTime);
      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(2);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)0));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)0));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(ts));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)0));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)0));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)0));

      ASSERT_from_PktSend(1,comBuff,(U32)0);

      // ignored channel

      buff.resetSer();
      ts.add(1,0);
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)20));
      this->invoke_to_TlmRecv(0,25,ts,buff);

      this->m_testTime.add(1,0);
      this->setTestTime(this->m_testTime);
      this->clearFromPortHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      // no packets should be pushed
      ASSERT_from_PktSend_SIZE(0);

  }

  void Tester ::
     sendManualPacketTest(void)
  {
      this->component.setPacketList(packetList,ignore,2);
      Fw::Time ts;
      Fw::TlmBuffer buff;

      // first channel
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U32)20));
      this->invoke_to_TlmRecv(0,10,ts,buff);

      // second channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)15));
      this->invoke_to_TlmRecv(0,100,ts,buff);

      // third channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)14));
      this->invoke_to_TlmRecv(0,333,ts,buff);

      // fifth channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U64)1000000));
      this->invoke_to_TlmRecv(0,13,ts,buff);

      // sixth channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)1010));
      this->invoke_to_TlmRecv(0,250,ts,buff);

      // seventh channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)15));
      this->invoke_to_TlmRecv(0,22,ts,buff);

      this->setTestTime(this->m_testTime);
      // run scheduler port to send packets
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      ASSERT_FROM_PORT_HISTORY_SIZE(2);
      ASSERT_from_PktSend_SIZE(2);

      // construct the packet buffers and make sure they are correct

      Fw::ComBuffer comBuff1;
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize(this->m_testTime));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize((U16)15));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize((U8)14));

      ASSERT_from_PktSend(0,comBuff1,(U32)0);

      Fw::ComBuffer comBuff2;
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff2.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff2.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff2.serialize(this->m_testTime));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff2.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff2.serialize((U64)1000000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff2.serialize((U16)1010));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff2.serialize((U8)15));

      ASSERT_from_PktSend(1,comBuff2,(U32)0);

      // should not be any new packets
      this->clearHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      ASSERT_FROM_PORT_HISTORY_SIZE(0);
      ASSERT_from_PktSend_SIZE(0);

      // send command to manually send a packet
      this->sendCmd_TPK_SEND_PKT(0,12,4);
      this->component.doDispatch();
      ASSERT_EVENTS_SIZE(1);
      ASSERT_EVENTS_TPK_PacketSent(0,4);
      ASSERT_CMD_RESPONSE_SIZE(1);
      ASSERT_CMD_RESPONSE(0,TlmPacketizerComponentBase::OPCODE_TPK_SEND_PKT,12,Fw::COMMAND_OK);
      // dispatch run call to send packet
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(1);
      ASSERT_from_PktSend(0,comBuff1,(U32)0);

      // another packet
      this->clearHistory();
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      ASSERT_FROM_PORT_HISTORY_SIZE(0);
      ASSERT_from_PktSend_SIZE(0);

      // send command to manually send a packet
      this->clearHistory();
      this->sendCmd_TPK_SEND_PKT(0,12,8);
      this->component.doDispatch();
      ASSERT_EVENTS_SIZE(1);
      ASSERT_EVENTS_TPK_PacketSent(0,8);
      ASSERT_CMD_RESPONSE_SIZE(1);
      ASSERT_CMD_RESPONSE(0,TlmPacketizerComponentBase::OPCODE_TPK_SEND_PKT,12,Fw::COMMAND_OK);
      // dispatch run call to send packet
      this->invoke_to_Run(0,0);
      this->component.doDispatch();
      ASSERT_from_PktSend_SIZE(1);
      ASSERT_from_PktSend(0,comBuff2,(U32)0);

      // Try to send invalid packet
      // send command to manually send a packet
      this->clearHistory();
      this->sendCmd_TPK_SEND_PKT(0,12,20);
      this->component.doDispatch();
      ASSERT_EVENTS_SIZE(1);
      ASSERT_EVENTS_TPK_PacketNotFound(0,20);
      ASSERT_CMD_RESPONSE_SIZE(1);
      ASSERT_CMD_RESPONSE(0,TlmPacketizerComponentBase::OPCODE_TPK_SEND_PKT,12,Fw::COMMAND_VALIDATION_ERROR);

  }

  void Tester ::
     setPacketLevelTest(void)
  {
      this->component.setPacketList(packetList,ignore,0);
      Fw::Time ts;
      Fw::TlmBuffer buff;

      // first channel
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U32)0x20));
      this->invoke_to_TlmRecv(0,10,ts,buff);

      // second channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)0x15));
      this->invoke_to_TlmRecv(0,100,ts,buff);

      // third channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)0x14));
      this->invoke_to_TlmRecv(0,333,ts,buff);

      // fifth channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U64)0x1000000));
      this->invoke_to_TlmRecv(0,13,ts,buff);

      // sixth channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)0x1010));
      this->invoke_to_TlmRecv(0,250,ts,buff);

      // seventh channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)0x15));
      this->invoke_to_TlmRecv(0,22,ts,buff);

      this->setTestTime(this->m_testTime);
      // run scheduler port to send packets
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      // should be no packets sent since packet level is 0
      ASSERT_FROM_PORT_HISTORY_SIZE(0);
      ASSERT_from_PktSend_SIZE(0);

      // send the command to select packet level 1
      this->clearHistory();
      this->sendCmd_TPK_SET_LEVEL(0,13,1);
      this->component.doDispatch();
      ASSERT_EVENTS_SIZE(1);
      ASSERT_EVENTS_TPK_LevelSet_SIZE(1);
      ASSERT_EVENTS_TPK_LevelSet(0,1);
      ASSERT_TLM_SIZE(1);
      ASSERT_TLM_TPK_SendLevel_SIZE(1);
      ASSERT_TLM_TPK_SendLevel(0,1);

      // send the packets
      // first channel
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U32)0x20));
      this->invoke_to_TlmRecv(0,10,ts,buff);

      // second channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)0x15));
      this->invoke_to_TlmRecv(0,100,ts,buff);

      // third channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)0x14));
      this->invoke_to_TlmRecv(0,333,ts,buff);

      // fifth channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U64)0x1000000));
      this->invoke_to_TlmRecv(0,13,ts,buff);

      // sixth channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U16)0x1010));
      this->invoke_to_TlmRecv(0,250,ts,buff);

      // seventh channel
      buff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,buff.serialize((U8)0x15));
      this->invoke_to_TlmRecv(0,22,ts,buff);

      this->setTestTime(this->m_testTime);
      // run scheduler port to send packets
      this->invoke_to_Run(0,0);
      this->component.doDispatch();

      // should be one packet sent since packet level is 1
      ASSERT_FROM_PORT_HISTORY_SIZE(1);
      ASSERT_from_PktSend_SIZE(1);

      // Should be packet 4

      Fw::ComBuffer comBuff1;
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize(this->m_testTime));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize((U32)0x20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize((U16)0x15));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff1.serialize((U8)0x14));

      ASSERT_from_PktSend(0,comBuff1,(U32)0);

return;

      ASSERT_FROM_PORT_HISTORY_SIZE(2);
      ASSERT_from_PktSend_SIZE(2);

      // construct the packet buffers and make sure they are correct

      Fw::ComBuffer comBuff;
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(4)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(this->m_testTime));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)15));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)14));

      ASSERT_from_PktSend(0,comBuff,(U32)0);

      comBuff.resetSer();
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwPacketDescriptorType>(Fw::ComPacket::FW_PACKET_PACKETIZED_TLM)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(static_cast<FwTlmPacketizeIdType>(8)));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize(this->m_testTime));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U32)20));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U64)1000000));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U16)1010));
      ASSERT_EQ(Fw::FW_SERIALIZE_OK,comBuff.serialize((U8)15));

      ASSERT_from_PktSend(1,comBuff,(U32)0);

  }

  void Tester ::
    nonPacketizedChannelTest(void) {

      this->component.setPacketList(packetList,ignore,2);
      Fw::Time ts;
      Fw::TlmBuffer buff;

      // start at non-used channel
      for (NATIVE_UINT_TYPE channel = 1000; channel < 1000 + TLMPACKETIZER_MAX_MISSING_TLM_CHECK; channel++) {
          this->clearEvents();
          this->invoke_to_TlmRecv(0,channel,ts,buff);
          ASSERT_EVENTS_SIZE(1);
          ASSERT_EVENTS_TPK_NoChan_SIZE(1);
          ASSERT_EVENTS_TPK_NoChan(0,channel);
      }

      // One more channel should not emit event
      this->clearEvents();
      this->invoke_to_TlmRecv(0,1000+TLMPACKETIZER_MAX_MISSING_TLM_CHECK,ts,buff);
      ASSERT_EVENTS_SIZE(0);
      ASSERT_EVENTS_TPK_NoChan_SIZE(0);

      // sending the missing channels again should emit no events

      for (NATIVE_UINT_TYPE channel = 1000; channel < 1000 + TLMPACKETIZER_MAX_MISSING_TLM_CHECK; channel++) {
          this->clearEvents();
          this->invoke_to_TlmRecv(0,channel,ts,buff);
          ASSERT_EVENTS_SIZE(0);
          ASSERT_EVENTS_TPK_NoChan_SIZE(0);
      }

  }

  void Tester ::
    pingTest(void) {

      this->component.setPacketList(packetList,ignore,2);
      // ping component
      this->clearFromPortHistory();
      this->invoke_to_pingIn(0,(U32)0x1234);
      this->component.doDispatch();
      ASSERT_from_pingOut_SIZE(1);
      ASSERT_from_pingOut(0,(U32)0x1234);

  }

  // ----------------------------------------------------------------------
  // Handlers for typed from ports
  // ----------------------------------------------------------------------

  void Tester ::
    from_Time_handler(
        const NATIVE_INT_TYPE portNum,
        Fw::Time &time
    )
  {
      time = this->m_testTime;
      this->m_timeSent = true;
  }

  void Tester ::
    from_PktSend_handler(
        const NATIVE_INT_TYPE portNum,
        Fw::ComBuffer &data,
        U32 context
    )
  {
    this->pushFromPortEntry_PktSend(data, context);
  }

  void Tester ::
    from_pingOut_handler(
        const NATIVE_INT_TYPE portNum,
        U32 key
    )
  {
    this->pushFromPortEntry_pingOut(key);
  }

  // ----------------------------------------------------------------------
  // Helper methods 
  // ----------------------------------------------------------------------

  void Tester ::
    connectPorts(void) 
  {

    // TlmRecv
    this->connect_to_TlmRecv(
        0,
        this->component.get_TlmRecv_InputPort(0)
    );

    // Run
    this->connect_to_Run(
        0,
        this->component.get_Run_InputPort(0)
    );

    // pingIn
    this->connect_to_pingIn(
        0,
        this->component.get_pingIn_InputPort(0)
    );

    // CmdDisp
    this->connect_to_CmdDisp(
        0,
        this->component.get_CmdDisp_InputPort(0)
    );

    // PktSend
    this->component.set_PktSend_OutputPort(
        0, 
        this->get_from_PktSend(0)
    );

    // pingOut
    this->component.set_pingOut_OutputPort(
        0, 
        this->get_from_pingOut(0)
    );

    // CmdStatus
    this->component.set_CmdStatus_OutputPort(
        0,
        this->get_from_CmdStatus(0)
    );

    // CmdReg
    this->component.set_CmdReg_OutputPort(
        0,
        this->get_from_CmdReg(0)
    );

    // Tlm
    this->component.set_Tlm_OutputPort(
        0,
        this->get_from_Tlm(0)
    );

    // Time
    this->component.set_Time_OutputPort(
        0,
        this->get_from_Time(0)
    );

    // Log
    this->component.set_Log_OutputPort(
        0,
        this->get_from_Log(0)
    );

    // LogText
    this->component.set_LogText_OutputPort(
        0,
        this->get_from_LogText(0)
    );

  }

  void Tester::textLogIn(const FwEventIdType id, //!< The event ID
          Fw::Time& timeTag, //!< The time
          const Fw::TextLogSeverity severity, //!< The severity
          const Fw::TextLogString& text //!< The event string
          ) {
      TextLogEntry e = { id, timeTag, severity, text };

      printTextLogHistoryEntry(e, stdout);
  }


  void Tester ::
    initComponents(void) 
  {
    this->init();
    this->component.init(
        QUEUE_DEPTH, INSTANCE
    );
  }

} // end namespace Svc

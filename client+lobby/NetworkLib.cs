﻿using UnityEngine;
using System.Net.Sockets;
using System.Threading;

public class NetworkLib
{
    private Socket mSocket;
    private PacketQueue mSendQueue;
    private PacketQueue mRecvQueue;

    // 이벤트 통지 델리게이트.
    public delegate void EventHandler(NetEventState state);
    // 이벤트 핸들러.
    private EventHandler mHandler;

    private System.Object mLockObj;

    public NetworkLib()
    {
        mSocket = null;
        mSendQueue = new PacketQueue();
        mRecvQueue = new PacketQueue();

        mLockObj = new System.Object();
    }

    public bool Connect(string address, int port)
    {
        try
        {
            lock (mLockObj)
            {
                mSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                mSocket.NoDelay = true;
                mSocket.SendBufferSize = 0;
                mSocket.Connect(address, port);
            }
        }
        catch
        {
            Debug.Log("Network::Connect fail!");
            return false;
        }

        Debug.Log("Connect::Called!!");
        return true;
    }

    public bool Disconnect()
    {
        try
        {
            if (null != mSocket)
            {
                lock (mLockObj)
                {
                    mSocket.Shutdown(SocketShutdown.Both);
                    // Socket 객체가 데이터를 전송하고 수신하는 기능을 비활성화한다.
                    // 파라미터는 Send, Receive, Both가 있다.
                    mSocket.Close();
                    mSocket = null;
                }
            }

            // 접속 종료를 알립니다.
            if (mHandler != null)
            {
                NetEventState state = new NetEventState();
                state.mType = NetEventType.Disconnect;
                state.mResult = NetEventResult.Success;
                mHandler(state);
            }

        }
        catch
        {
            Debug.Log("Network::Disonnect fail!");
            return false;
        }

        Debug.Log("Disconnect::Called!!");
        return true;
    }

    // thread start function
    public void Dispatch()
    {
        //Debug.Log("Dispatch::Running!");
        lock (mLockObj)
        {
            DispatchSend();

            DispatchReceive();
        }

        return;
    }

    private void DispatchSend()
    {
        if (null == mSocket) return;

        try
        {
            byte[] buffer = new byte[(int)NetConfig.MAX_BUFFER_SIZE];

            int nSendSize = mSendQueue.Dequeue(ref buffer, buffer.Length);

            while (nSendSize > 0)
            {
                mSocket.Send(buffer, nSendSize, SocketFlags.None);
                nSendSize = mSendQueue.Dequeue(ref buffer, buffer.Length);
            }
        }
        catch
        {
            if (mHandler != null)
            {
                NetEventState state = new NetEventState();
                state.mType = NetEventType.SendError;
                state.mResult = NetEventResult.Failure;
                mHandler(state);
            }
        }
        return;
    }

    private void DispatchReceive()
    {
        if (null == mSocket) return;

        try
        {
            if (mSocket.Poll(0, SelectMode.SelectRead))
            {
                byte[] buffer = new byte[(int)NetConfig.MAX_BUFFER_SIZE];
                int nRecvSize = mSocket.Receive(buffer, buffer.Length, SocketFlags.None);
                if (nRecvSize == 0)
                {
                    // disconnect and exit
                    Disconnect();
                }
                else if (nRecvSize > 0)
                {
                    mRecvQueue.Enqueue(buffer, nRecvSize);
                }
            }
        }
        catch
        {
            if (mHandler != null)
            {
                NetEventState state = new NetEventState();
                state.mType = NetEventType.ReceiveError;
                state.mResult = NetEventResult.Failure;
                mHandler(state);
            }
        }

        return;
    }

    public int Send(byte[] data, int size)
    {
        return mSendQueue.Enqueue(data, size);
    }

    public int Receive(ref byte[] buffer, int size)
    {
        return mRecvQueue.Dequeue(ref buffer, size);
    }

    // 이벤트 통지 함수 등록.
    public void RegisterEventHandler(EventHandler handler)
    {
        mHandler += handler;
        return;
    }

    // 이벤트 통지 함수 삭제.
    public void UnregisterEventHandler(EventHandler handler)
    {
        mHandler -= handler;
        return;
    }
}
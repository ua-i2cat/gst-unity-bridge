using UnityEngine;
using System;
using System.Collections.Generic;

/*
 * Event processor to ensure events are executed from the main thread.
 * http://stackoverflow.com/questions/22513881/unity3d-how-to-process-events-in-the-correct-thread
 */

public class EventProcessor : MonoBehaviour
{

	public void QueueEvent(Action action)
	{
		lock (m_queueLock)
		{
			m_queuedEvents.Add(action);
		}
	}

	void Update()
	{
		MoveQueuedEventsToExecuting();

		while (m_executingEvents.Count > 0)
		{
			Action e = m_executingEvents[0];
			m_executingEvents.RemoveAt(0);
			e();
		}
	}

	private void MoveQueuedEventsToExecuting()
	{
		lock (m_queueLock)
		{
			while (m_queuedEvents.Count > 0)
			{
				Action e = m_queuedEvents[0];
				m_executingEvents.Add(e);
				m_queuedEvents.RemoveAt(0);
			}
		}
	}

	private object m_queueLock = new object();
	private List<Action> m_queuedEvents = new List<Action>();
	private List<Action> m_executingEvents = new List<Action>();
}

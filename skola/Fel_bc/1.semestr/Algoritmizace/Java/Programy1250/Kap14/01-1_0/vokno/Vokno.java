/* Soubor Kap14\01-1_0\vokno\Vokno.java
 * Prvn� okno
 * aby se program choval p�i uzav�en� okna korektn� (aby skon�il),
 * je t�eba upravit reakci na ud�lost, kter� nastane p�i
 * uzav�en� okna.
 * Program ukazuje �e�en� pou�iteln� ve v�ech verz�ch Javy:
 * povoluje v konstruktoru okna "okenn�" ud�losti a p�edefinov�v�
 * chr�n�nou metodu processWindowEvent().
 */

package vokno;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class Vokno extends JFrame
{
	// �e�en� pro JDK 1.0 a pozd�j��
	public Vokno()	
	{
		enableEvents(AWTEvent.WINDOW_EVENT_MASK);
	}
	
	protected void processWindowEvent(WindowEvent e) {
		super.processWindowEvent(e);
		if (e.getID() == WindowEvent.WINDOW_CLOSING) {
			System.exit(0);	
		}	
	}
}
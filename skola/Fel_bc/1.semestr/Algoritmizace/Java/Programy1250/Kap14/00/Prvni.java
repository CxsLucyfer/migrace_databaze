// Soubor Kap14\00\Prvni.java
// Deklaruje t��du okna, kter� je 
// potomken JFrame, vytvo�� instanci a zobraz� ji.
// Program vytvo�� minim�ln� okno
// a po uzav�en� okna neskon��, program nutno
// ukon�it v konzolov�m okn� kl�vesovou
// kombinac� Ctrl+C


import javax.swing.*;
class Vokno extends JFrame
{
	Vokno(){}		// Vol� implicitn� konstruktor p�edka, nic v�c
}

public class Prvni
{
	public static void main(String[] a)
	{
		Vokno okno = new Vokno();	// Vytvo� okno
		okno.setVisible(true);						// a zobraz ho
	}
}

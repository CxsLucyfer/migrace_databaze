/* Soubor Kap14\01-1_2\Prvni.java
 * Vytvo�� okno a zobraz� ho
 * 
 * Ukon�en� programu p�i uzav�en� hlavn�ho okna
 * V koment��i je �e�en� spole�n� pro JDK 1.2 a nov�j��
 * za n�m n�sleduje �e�en�, kter� lze pou��t jen v JDK 1.3 a nov�j��ch
 *
 * �e�en� v JDK 1.2 je zalo�eno na  
 * anonymn� instanci potomka t��dy WindowAdapter, 
 * kter� se zaregistruje jako
 * p��jemce okenn�ch ud�lost� a p�i uzav�en� okna zavol� System.exit().
 *
 * �e�en� v JDK 1.3 spo��v� ve vol�n� metody okna
 * setDefaultCloseOperation(okno.EXIT_ON_CLOSE)
 */
	
public class Prvni
{
	public static void main(String[] a)
	{
		vokno.Vokno okno = new vokno.Vokno();
		okno.setVisible(true);
		// �e�en� pro JDK 1.2
		/*
		okno.addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent e) {
				System.exit(0);
			}
		});
		*/
		okno.setDefaultCloseOperation(okno.EXIT_ON_CLOSE);
	}
}



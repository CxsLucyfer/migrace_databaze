/* Soubor Kap08\04\Continue.java
   p��klad na p��kaz Continue
   P�i vypisov�n� pole �et�zc� p�esko�� v�echny v�pisy 
   slova "necenzurovan�"
*/

public class Continue {
	String[] st= {"Toto", "je", "necenzurovan�", "�et�zec"};
	public void cenzura()
	{
		for(int i = 0; i < st.length; i++)
		{		
			if(st[i].equals("necenzurovan�")) continue;
			System.out.print(st[i]+ " ");
		}
	}
	public static void main(String[] s)
	{	
		Continue c = new Continue();
		c.cenzura();
	}
}
/* Soubor Kap11\02\Test.java
 * testovac� program k p��kladu na rozhran�
 * podobn� jako v kap11\01, nav�c implementuje klonov�n�
 ---------------------------------------------------------
 * vytvo�� kontejner, ulo�� do n�j n�kolik grafick�ch objekt�
 * a jednu instanci t��dy retezec
 * V�echny t��dy implementuj� rozkran� kontrola.Kontrola,
 * kter� obsahuje metodu zkontroluj().
 * Metoda kontrol t��dy Test projde kontejner,
 * odkaz na ka�d� z objekt� p�etypuje na odkaz na rozhran� Kontrola a zavol�
 * metodu kontrola. Tak zkontroluje v�echny objekty bez ohledu na to, zda jde o
 * �et�zce nebo grafick� objekty. V�sledek kontrolu kontejnery vyp�e.
 */

import grafika.*;
import kontrola.*;
import retezec.*;
import java.util.*;


public class Test {
  ArrayList seznam = new ArrayList();

  public Test() { }


  public boolean kontrol(ArrayList s){		// Zkontroluj obsah kontejneru
    try{					// pomoc� metody kontrola() z
      for(Iterator i = s.iterator(); i.hasNext();) // rozhran� Kontrola
      {
        Kontrola k = (Kontrola)i.next();
        System.out.println(k);
	if(!(k.zkontroluj())) return false;
      }
      return true;
    }
    catch(Exception e){
     e.printStackTrace();
     return false;
    }
  }

  public void beh() // Napln� kontejner n�jak�mi objekty
  {
    GO g = new Bod(5,6,11);
    seznam.add(g);
    g = new Usecka(1,2,3,4,5);
    seznam.add(g);
    g = new Kruznice(8, 9, 10, 98);
    seznam.add(g);
    Retezec s = new Retezec();
    s.pridej("Nejaky Text");
    seznam.add(s);

    System.out.println(seznam);	
    ArrayList seznam1 = new ArrayList();	// Vytvo� nov� seznam
    for(Iterator i = seznam.iterator(); i.hasNext();)// a napl� ho kopiemi
    {						// objekt� z p�edchoz�ho seznamu:
      try {
        g = (GO)i.next();			// Vezmi dal�� objekt
        g = (GO)g.clone();			// Klonuj ho
        g.setBarva(123);			// Zm�� barvu kopie
        seznam1.add(g);				// Ulo� ho do nov�ho seznamu
      }
      catch(Exception e)			// Kdy� se n�co nepoda��, nic se ned�je
      {}
    }
    System.out.println(seznam1);		// Vypi� v�sledek
  }



  public static void main(String[] args) {
    Test t = new Test();
    t.beh();
  }
}
/* Soubor Kap11\01\Test.java
 * testovac� program k p��kladu na rozhran�
 * vytvo�� kontejner, ulo�� do n�j n�kolik grafick�ch objekt�
 * a jednu instanci t��dy retezec
 * V�echny t��dy implementuj� rozkran� kontrola.Kontrola,
 * kter� obsahuje metodu zkontroluj().
 * Metoda kontrol t��dy Test projde kontejner,
 * odkaz na ka�d� z objekt� p�etypuje na odkaz na rozhran� Kontrola a zavol�
 * metodu kontrola. Tak zkontroluje v�echny objekty bez ohledu na to, zda jde o
 * �et�zce nebo grafick� objekty. V�sledek kontrolu kontejnery vyp�e.
 * Vhodn� pro v�echny verze JDK, nebo� nevyu��v� parametrizace kontejneru 
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
    System.out.println(kontrol(seznam)?"OK":"Prusvih");
  }


  public static void main(String[] args) {
    Test t = new Test();
    t.beh();
  }
}
/* Soubor Kap13\03\Test.java
 * testovac� program k p��kladu na serializaci
 * podobn� jako v kap11\02, nav�c implementuje serializaci
 ---------------------------------------------------------
 * vytvo�� kontejner, ulo�� do n�j n�kolik grafick�ch objekt�
 * a jednu instanci t��dy retezec
 * V�echny t��dy implementuj� rozkran� kontrola.Kontrola,
 * kter� obsahuje metodu zkontroluj(), rozhran� Cloneable, kter� umo��uje
 * klonov�n�, a rozhran� Serializable, kter� umo��uje serializaci.
 *
 */

import grafika.*;
import kontrola.*;
import java.util.*;
import java.io.*;


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

  public void uloz(String soubor, Object co) throws IOException
  {
    File f = new File(soubor);
    if(!f.exists()) f.createNewFile();

    FileOutputStream fos = new FileOutputStream(f);
    ObjectOutputStream oos = new ObjectOutputStream(fos);

    oos.writeObject(co);
    oos.close();
  }

  public Object nacti(String soubor) throws IOException, ClassNotFoundException
  {
    File f = new File(soubor);
    if(!f.exists()) throw new IOException("soubor "+ soubor + " neexistuje");

    FileInputStream fis = new FileInputStream(f);
    ObjectInputStream ois = new ObjectInputStream(fis);

    return ois.readObject();
  }
  public void beh() // Napln� kontejner n�jak�mi objekty
  {
    GO g = new Bod(5,6,11);
    seznam.add(g);
    g = new Usecka(1,2,3,4,5);
    seznam.add(g);
    g = new Kruznice(8, 9, 10, 98);
    seznam.add(g);

    try {
     uloz("data.dta", seznam);
     System.out.println(seznam);
     seznam = null;
     seznam = (ArrayList) nacti("data.dta");
     System.out.println(seznam);
    }
    catch(Exception e)
    {
      System.out.println(e);
      System.out.println("Neco se nepovedlo");
    }
  }



  public static void main(String[] args) {
    Test t = new Test();
    t.beh();
  }
}
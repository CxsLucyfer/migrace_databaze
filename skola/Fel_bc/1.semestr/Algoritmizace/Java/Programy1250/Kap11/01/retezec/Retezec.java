package retezec;

/* Soubor Kap11\01\Retezec.java
 * Implementace m�niteln�ho �et�zce s mo�nost� kontroly.
 * Proto�e t��dy String i StringBuffer jsou kone�n� (final),
 * nelze je pou��t jako p�edka. Proto zapouzd��me instanci t��dy
 * StringBuffer do instance t��dy Retezec a obal�me vlastn�mi metodami.

 * Implementuje rozhran� kontrola.Kontrola.
 */

import kontrola.*;

public class Retezec implements Kontrola  {
  StringBuffer str = new StringBuffer("");
  public Retezec() { }

  String getString() { return str.toString(); }

  public void pridej(String s) {
    str.append(s);
  }

  public boolean zkontroluj() {
      return str != null;
  }

}
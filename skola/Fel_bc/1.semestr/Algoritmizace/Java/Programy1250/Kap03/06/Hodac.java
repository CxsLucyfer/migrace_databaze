class Text {
 private String text;
 public Text()
 {
   text = "Vtipn� text";
 }
 public Text(String s) {text = s;}
 public void zmenText(String s) {text = s;}
 public void vypis() { System.out.println(text);}
}

class Napis
{
  public static void main(String[] arg)
  {
	Text n1 = new Text();
	Text n2 = new Text("Vesel� v�noce");
	n1.vypis();
	Text n3 = n1;
	n3.vypis();
	n2.vypis();
	n1.zmenText("Gener�ln� �editel �T Ji�� Hoda� se obr�til"+
		" na Radu �esk� republiky pro rozhlasov� a televizn� " +
		"vys�l�n� se ��dost� o rozhodnut�, kter� program �T " +
		"je leg�ln�m a autorizovan�m programem v souladu se z�konem " +
		"o �esk� televizi a kter� nikoli. Do rozhodnut� Rady vys�l� �T " +
		"jako sv�j program toto sd�len�.");

	n1.vypis();
	n3.vypis();
  }
}
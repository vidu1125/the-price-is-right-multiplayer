from app.models import db

class Question(db.Model):
    __tablename__ = "questions"

    id = db.Column(db.Integer, primary_key=True)
    text = db.Column(db.Text)
    image = db.Column(db.Text)
    a = db.Column(db.Text)
    b = db.Column(db.Text)
    c = db.Column(db.Text)
    d = db.Column(db.Text)
    correct = db.Column(db.String(1))
    active = db.Column(db.Boolean)
    last_updated_by = db.Column(db.Integer, db.ForeignKey("accounts.id", ondelete="SET NULL"))
    last_updated_at = db.Column(db.DateTime)

    def to_dict(self):
        return {
            "id": self.id,
            "text": self.text,
            "image": self.image,
            "a": self.a,
            "b": self.b,
            "c": self.c,
            "d": self.d,
            "correct": self.correct,
            "active": self.active,
        }

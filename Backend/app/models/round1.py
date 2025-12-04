from app.models import db

class R1Item(db.Model):
    __tablename__ = "r1_items"

    id = db.Column(db.Integer, primary_key=True)
    round_id = db.Column(db.Integer, db.ForeignKey("rounds.id", ondelete="CASCADE"))
    idx = db.Column(db.Integer)
    text = db.Column(db.Text)
    image = db.Column(db.Text)
    a = db.Column(db.Text)
    b = db.Column(db.Text)
    c = db.Column(db.Text)
    d = db.Column(db.Text)
    correct = db.Column(db.String(1))

    def to_dict(self):
        return {
            "id": self.id,
            "round_id": self.round_id,
            "idx": self.idx,
            "text": self.text,
            "image": self.image,
            "a": self.a,
            "b": self.b,
            "c": self.c,
            "d": self.d,
            "correct": self.correct,
        }

class R1Answer(db.Model):
    __tablename__ = "r1_answers"

    id = db.Column(db.Integer, primary_key=True)
    item_id = db.Column(db.Integer, db.ForeignKey("r1_items.id", ondelete="CASCADE"))
    player_id = db.Column(db.Integer, db.ForeignKey("match_players.id", ondelete="CASCADE"))
    ans = db.Column(db.String(1))
    time_ms = db.Column(db.Integer)
    correct = db.Column(db.Boolean)
    points = db.Column(db.Integer)

    def to_dict(self):
        return {
            "id": self.id,
            "item_id": self.item_id,
            "player_id": self.player_id,
            "ans": self.ans,
            "time_ms": self.time_ms,
            "correct": self.correct,
            "points": self.points
        }

from app.models import db

class R3Item(db.Model):
    __tablename__ = "r3_items"

    id = db.Column(db.Integer, primary_key=True)
    round_id = db.Column(db.Integer, db.ForeignKey("rounds.id", ondelete="CASCADE"))
    value = db.Column(db.Integer)

    def to_dict(self):
        return {
            "id": self.id,
            "round_id": self.round_id,
            "value": self.value,
        }
class R3Answer(db.Model):
    __tablename__ = "r3_answer"

    id = db.Column(db.Integer, primary_key=True)
    round_id = db.Column(db.Integer, db.ForeignKey("rounds.id", ondelete="CASCADE"))
    player_id = db.Column(db.Integer, db.ForeignKey("match_players.id", ondelete="CASCADE"))
    spin1_value = db.Column(db.Integer)
    spin2_value = db.Column(db.Integer)
    points = db.Column(db.Integer)

    def to_dict(self):
        return {
            "id": self.id,
            "round_id": self.round_id,
            "player_id": self.player_id,
            "spin1_value": self.spin1_value,
            "spin2_value": self.spin2_value,
            "points": self.points,
        }

from app.models import db

class R2Item(db.Model):
    __tablename__ = "r2_items"

    id = db.Column(db.Integer, primary_key=True)
    round_id = db.Column(db.Integer, db.ForeignKey("rounds.id", ondelete="CASCADE"))
    name = db.Column(db.String(255))
    description = db.Column(db.Text)
    image = db.Column(db.String(255))
    price = db.Column(db.Integer)
    idx = db.Column(db.Integer)

    def to_dict(self):
        return {
            "id": self.id,
            "round_id": self.round_id,
            "name": self.name,
            "description": self.description,
            "image": self.image,
            "price": self.price,
            "idx": self.idx
        }
class R2Answer(db.Model):
    __tablename__ = "r2_answers"

    id = db.Column(db.Integer, primary_key=True)
    item_id = db.Column(db.Integer, db.ForeignKey("r2_items.id", ondelete="CASCADE"))
    player_id = db.Column(db.Integer, db.ForeignKey("match_players.id", ondelete="CASCADE"))
    bid = db.Column(db.Integer)
    overbid = db.Column(db.Boolean)
    points = db.Column(db.Integer)
    wager = db.Column(db.Boolean)

    def to_dict(self):
        return {
            "id": self.id,
            "item_id": self.item_id,
            "player_id": self.player_id,
            "bid": self.bid,
            "overbid": self.overbid,
            "points": self.points,
            "wager": self.wager,
        }

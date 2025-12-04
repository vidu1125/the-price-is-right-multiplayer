from app.models import db

class PlayerMatchHistory(db.Model):
    __tablename__ = "player_match_history"

    id = db.Column(db.Integer, primary_key=True)
    player_id = db.Column(db.Integer, db.ForeignKey("accounts.id", ondelete="CASCADE"))
    match_id = db.Column(db.Integer, db.ForeignKey("matches.id", ondelete="CASCADE"))
    created_at = db.Column(db.DateTime)

    def to_dict(self):
        return {
            "id": self.id,
            "player_id": self.player_id,
            "match_id": self.match_id,
            "created_at": self.created_at.isoformat() if self.created_at else None
        }

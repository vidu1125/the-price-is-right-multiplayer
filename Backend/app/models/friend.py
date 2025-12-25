from app.models import db
from .base import Base
from sqlalchemy.sql import func

class Friend(Base):
    __tablename__ = "friends"

    id = db.Column(db.Integer, primary_key=True)

    requester_id = db.Column(
        db.Integer,
        db.ForeignKey("accounts.id"),
        nullable=False
    )

    addressee_id = db.Column(
        db.Integer,
        db.ForeignKey("accounts.id"),
        nullable=False
    )

    status = db.Column(db.String(20), nullable=False)

    created_at = db.Column(db.DateTime, server_default=func.now())
    updated_at = db.Column(db.DateTime, onupdate=func.now())

    __table_args__ = (
        db.UniqueConstraint("requester_id", "addressee_id", name="uq_friend_pair"),
    )
